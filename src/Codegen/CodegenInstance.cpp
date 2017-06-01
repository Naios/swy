
/**
  Copyright(c) 2016 - 2017 Denis Blank <denis.blank at outlook dot com>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**/

#include "CodegenInstance.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "AST.hpp"
#include "ASTContext.hpp"
#include "ASTTraversal.hpp"
#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"
#include "CompilerInvocation.hpp"
#include "DependencyAnalysis.hpp"
#include "FunctionCodegen.hpp"
#include "MetaCodegen.hpp"

static std::unique_ptr<llvm::Module>
createModule(llvm::LLVMContext& context,
             CompilationUnit const* compilationUnit) {

  auto name = compilationUnit->getSourceFileName();
  auto module = std::make_unique<llvm::Module>(name, context);

  module->setSourceFileName(compilationUnit->getSourceFilePath());

  // Set the data layout for the host machine
  auto machine = compilationUnit->getCompilerInstance()->getTargetMachine();
  module->setDataLayout(machine->createDataLayout());

  module->setTargetTriple(machine->getTargetTriple().getTriple());
  return module;
}

static std::unique_ptr<llvm::legacy::FunctionPassManager>
createFunctionPassManager(llvm::Module* module,
                          CompilerInvocation const* invocation) {

  auto manager = std::make_unique<llvm::legacy::FunctionPassManager>(module);

  llvm::PassManagerBuilder builder;

  // We verify the input
  builder.VerifyInput = true;

  builder.OptLevel = unsigned(invocation->getOptLevel());
  builder.SizeLevel = 0;

  // builder.Inliner = llvm::createFunctionInliningPass(builder.OptLevel,
  // builder.SizeLevel);
  builder.LoopVectorize = false;

  // Populate the manager
  builder.populateFunctionPassManager(*manager);
  return manager;
}

CodegenInstance::CodegenInstance(CompilationUnit* compilationUnit,
                                 ASTContext* astContext)
    : compilationUnit_(compilationUnit), astContext_(astContext),
      llvmContext_(std::make_unique<llvm::LLVMContext>()),
      amalgamation_(createModule(*llvmContext_, compilationUnit)),
      passManager_(createFunctionPassManager(
          amalgamation_.get(),
          compilationUnit->getCompilerInstance()->getInvocation())) {}

CodegenInstance::~CodegenInstance() {
  /// Reset the code executor first
  codeExecuter_.reset();
}

std::unique_ptr<CodegenInstance>
CodegenInstance::createFor(CompilationUnit* compilationUnit,
                           ASTContext* astContext) {
  std::unique_ptr<CodegenInstance> instance(
      new CodegenInstance(compilationUnit, astContext));

  auto executor = CodeExecutor::createFor(instance.get());
  if (!executor) {
    return nullptr;
  }

  instance->codeExecuter_.reset(new CodeExecutor(std::move(*executor)));
  return instance;
}

CompilationUnit* CodegenInstance::getCompilationUnit() {
  return compilationUnit_;
}

llvm::LLVMContext& CodegenInstance::getLLVMContext() {
  return *llvmContext_.get();
}

llvm::Module* CodegenInstance::getModule() { return amalgamation_.get(); }

void CodegenInstance::dump() { getModule()->print(llvm::outs(), nullptr); }

llvm::Constant*
CodegenInstance::lookupGlobal(FunctionDeclASTNode const* function) {

  auto global = llvm::cast<llvm::Function>(createFunctionPrototype(function));
  if (global->isDeclaration()) {
    // Add it to the list of the global dependencies to codegen
    dependencies_.insert(function);
  }

  functionSource_[global] = function;
  return global;
}

Nullable<llvm::Constant*>
CodegenInstance::lookupGlobal(MetaInstantiationExprASTNode const* inst,
                              bool requiresCompleted) {
  // Instantiate the decl and return a prototype to it
  return instantiate(inst, requiresCompleted)
      .map([&](auto node) -> Nullable<llvm::Constant*> {
        if (auto function = llvm::dyn_cast<FunctionDeclASTNode>(node)) {
          return lookupGlobal(function);
        } else {
          return lookupGlobal(llvm::cast<MetaInstantiationExprASTNode>(node),
                              requiresCompleted);
        }
      });
}

Nullable<llvm::Constant*>
CodegenInstance::resolveGlobal(llvm::Function const* function) {
  auto itr = functionSource_.find(function);
  assert(itr != functionSource_.end() &&
         "The function should be contained in the source map!");

  if (auto fn = itr->second.dyn_cast<FunctionDeclASTNode const*>()) {
    return resolveGlobal(fn);
  } else {
    return resolveGlobal(
        itr->second.get<MetaInstantiationExprASTNode const*>());
  }
}

Nullable<llvm::Constant*>
CodegenInstance::resolveGlobal(FunctionDeclASTNode const* function) {
  return codegen(function);
}

Nullable<llvm::Constant*>
CodegenInstance::resolveGlobal(MetaInstantiationExprASTNode const* inst) {
  return codegen(inst);
}

ScopeLeaveAction CodegenInstance::enterGeneration(ASTNode const* node) {
  // Find out whether the codegen is generating the function body already,
  // which indicates a strong unresolvable cycle.
  if (currentGeneration_.find(node) != currentGeneration_.end()) {
    llvm_unreachable("Cycle detected"); // TODO improve this
  }
  currentGeneration_.insert(node);
  return ScopeLeaveAction([=] { currentGeneration_.erase(node); });
}

bool CodegenInstance::codegen(
    CompilationUnitASTNode const* compilationUnitASTNode) {

  {
    assert(dependencies_.empty() &&
           "Expected to start with empty dependencies!");
    auto children = compilationUnitASTNode->children();
    dependencies_.insert(children.begin(), children.end());
  }

  // Codegen the dependencies of the global child nodes
  while (!dependencies_.empty()) {
    auto dependency = *dependencies_.begin();
    dependencies_.erase(dependency);
    bool ok = traverseNode(dependency,
                           [&](auto* node) { return bool(codegen(node)); });
    if (!ok) {
      return false;
    }
  }
  return true;
}

Nullable<llvm::Function*>
CodegenInstance::codegen(FunctionDeclASTNode const* node) {
  auto function = llvm::cast<llvm::Function>(lookupGlobal(node));
  if (!function->isDeclaration()) {
    // The function was generated already
    return function;
  }

  /// Make sure all meta instantiations are instantiated
  consumeMetaInstantiationsOf(node,
                              [&](MetaInstantiationExprASTNode const* inst) {
                                return bool(lookupGlobal(inst));
                              });

  if (!canContinue()) {
    return {};
  }

  if (llvm::isa<MetaUnitASTNode>(node->getContainingUnit())) {
    // For intermediate produces results set the linkage to private
    function->setLinkage(llvm::GlobalValue::PrivateLinkage);
  }

  // Set the argument names of the function
  {
    auto itr = node->getArgDeclList()->children().begin();
    auto range = function->args();

    /*
    if (isThisCall) {
      auto next = range.begin();
      next->setName("@@context@@");
      ++next;
      range = { next, range.end() };
    }
    */

    for (auto& arg : range) {
      auto current = *itr;
      if (auto named = llvm::dyn_cast<NamedArgumentDeclASTNode>(current)) {
        arg.setName(*named->getName());
      }
      ++itr;
    }
  }

  auto generation = enterGeneration(node);

  FunctionCodegen functionCodegen(this, function);
  functionCodegen.codegen(node);

  optimizeFunction(function);

  return function;
}

Nullable<llvm::Constant*>
CodegenInstance::codegen(MetaInstantiationExprASTNode const* node) {
  return instantiate(node).map([&](ASTNode const* exported) {
    auto matcher = pred::isAnyNodeOf(identityOf<FunctionDeclASTNode>() /*,
                                     identityOf<MetaDeclASTNode>()*/);

    return traverseNodeExpecting(
        exported, matcher, [&](auto promoted) -> Nullable<llvm::Constant*> {
          return codegen(promoted);
        });
  });
}

Nullable<ASTNode const*>
CodegenInstance::instantiate(MetaInstantiationExprASTNode const* inst,
                             bool requiresCompleted) {
  return codeExecuter_->instantiate(inst, requiresCompleted)
      .map([&](auto unit) -> Nullable<ASTNode const*> {
        if (auto exported = unit->getExportedNode()) {
          // Return a prototype to the exporting node
          return exported;
        } else {
          // Yield an error because we expect every used meta decl to export a
          // top level decl.
          getCompilationUnit()->getDiagnosticEngine()->diagnose(
              Diagnostic::ErrorInstantiationNoExport, inst->getSourceRange(),
              inst->getDecl()->getName());
          getCompilationUnit()->getDiagnosticEngine()->diagnose(
              Diagnostic::NoteDeclarationHint,
              inst->getDecl()->getDecl()->getName(),
              inst->getDecl()->getDecl()->getName());
          return {};
        }
      });
}

void CodegenInstance::optimizeFunction(llvm::Function* function) {
  // Just run the pass manager on the function
  passManager_->run(*function);
}
