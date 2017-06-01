
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

#include "CodeExecutor.hpp"

#include "llvm/ADT/Optional.h"
#include "llvm/IR/Function.h"
// Important header to link in MCJit
// Don't remove it!
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "ASTCloner.hpp"
#include "ASTCursor.hpp"
#include "ASTDumper.hpp"
#include "ASTLayout.hpp"
#include "ASTPredicate.hpp"
#include "CodegenInstance.hpp"
#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"
#include "CompilerInvocation.hpp"
#include "DependencyAnalysis.hpp"
#include "Formatting.hpp"
#include "FunctionCodegen.hpp"
#include "MetaCodegen.hpp"
#include "NonCopyable.hpp"
#include "SemaAnalysis.hpp"

static bool shouldPrintVerboseMsg(CompilationUnit* compilationUnit,
                                  VerboseFlag flag) {
  return compilationUnit->getCompilerInstance()
      ->getInvocation()
      ->hasVerboseFlag(flag);
}

static bool shouldPrintVerboseMsg(CodeExecutor* executor, VerboseFlag flag) {
  return shouldPrintVerboseMsg(executor->getCompilationUnit(), flag);
}

/// Returns the given instantiation as readable string
static std::string
stringifyInstantiation(MetaInstantiationExprASTNode const* inst) {
  llvm::SmallVector<std::string, 5> args;

  for (auto arg : inst->getArguments()) {
    if (auto intConstant = llvm::dyn_cast<IntegerLiteralExprASTNode>(arg)) {
      args.push_back(std::to_string(*intConstant->getLiteral()));
    } else {
      args.push_back("{intermediate}");
    }
  }
  return fmt::format("{}<{}>", *inst->getDecl()->getName(),
                     llvm::join(args.begin(), args.end(), ", "));
}

/// A class to consume node contributions from meta functions
class NodeContributor : public NonMovable {
  ASTLayoutWriter& writer_;
  CompilationUnit* compilationUnit_;
  MetaInstantiationExprASTNode const* inst_;
  SourceRelocator relocator_;
  ASTContext* context_;
  ASTCloner cloner_;

public:
  explicit NodeContributor(ASTLayoutWriter& writer,
                           CompilationUnit* compilationUnit,
                           MetaInstantiationExprASTNode const* inst,
                           ASTContext* context)
      : writer_(writer), compilationUnit_(compilationUnit), inst_(inst),
        context_(context), cloner_(context, &relocator_) {}

  /// Clone the node and write it to the new layout
  void contribute(ASTNode const* node) {
    auto cloned = cloner_.clone(node);
    writer_.directWrite(cloned);
  }

  /// Introduces a constant node on the base of the given node
  void introduce(ASTNode const* node, int value, ASTCursor const& cursor) {
    traverseNodeExpecting(
        node, pred::isNamedDeclContext(),
        [&](NamedDeclContext const* promoted) {
          // Print the exported values
          if (shouldPrintVerboseMsg(compilationUnit_,
                                    VerboseFlag::InstantiatedExports)) {
            compilationUnit_->getDiagnosticEngine()->diagnose(
                Diagnostic::NoteInstantiationExported, promoted->getName(),
                stringifyInstantiation(inst_), promoted->getName(), value);
          }

          auto name = relocator_.relocate(promoted->getName());
          // Just write a dummy decl into the layout
          if (cursor.isInsideFunctionDecl()) {
            /// Allocate a statament only when it is valid to use:
            /// which means when we are inside a function
            writer_.write(context_->allocate<DeclStmtASTNode>(name));
          } else {
            /// Contribute a global constant when we are in the top level scope
            writer_.write(context_->allocate<GlobalConstantDeclASTNode>(name));
          }

          RangeAnnotated<int> exported(value, name.getAnnotation());
          writer_.write(
              context_->allocate<IntegerLiteralExprASTNode>(exported));
        });
  }

  /// Reduce the current production
  void reduce() { writer_.markReduce(); }
};

/// Creates a named module for the given CodegenInstance
static std::unique_ptr<llvm::Module> createModule(IRContext* context,
                                                  llvm::StringRef name) {
  auto module = std::make_unique<llvm::Module>(name, context->getLLVMContext());
  module->setTargetTriple(llvm::sys::getDefaultTargetTriple());
  return module;
};

namespace {
template <typename T>
class FunctionValueMaterializer : public llvm::ValueMaterializer {
  T fn_;

public:
  explicit FunctionValueMaterializer(T fn) : fn_(fn) {}
  virtual ~FunctionValueMaterializer() = default;

  llvm::Value* materialize(llvm::Value* value) override { return fn_(value); }
};
}

/// Makes a ValueMaterializer from the given functor.
template <typename T> auto makeValueMaterializer(T&& function) {
  return FunctionValueMaterializer<std::decay_t<T>>(std::forward<T>(function));
}

static llvm::Function*
cloneFunctionIntoModule(llvm::Function* function, llvm::Module* target,
                        llvm::ValueMaterializer* materializer = nullptr) {
  auto cloned = llvm::cast<llvm::Function>(target->getOrInsertFunction(
      function->getName(), function->getFunctionType()));

  assert(cloned->isDeclaration() && "The function is existing already!");

  // Since we are using only 1 executor for every compilation unit
  // and we are cross sharing the globals across multiple modules
  // we need to set every function to an external linkage.
  cloned->setLinkage(llvm::GlobalValue::ExternalLinkage);

  /// Remap the function arguments
  llvm::ValueToValueMapTy mapping;
  auto itr = cloned->arg_begin();
  for (auto const& arg : function->args()) {
    itr->setName(arg.getName());
    mapping[&arg] = &*itr++;
  }

  // Ignore return instructions
  llvm::SmallVector<llvm::ReturnInst*, 5> returnMapper;

  llvm::CloneFunctionInto(cloned, function, mapping, true, returnMapper, "",
                          nullptr, nullptr, materializer);
  return cloned;
}

void CodeExecutor::setExecutor(
    std::unique_ptr<llvm::ExecutionEngine> executor) {
  assert(!executor_ && "Executor already set!");
  executor_ = move(executor);
}

llvm::Optional<CodeExecutor> CodeExecutor::createFor(IRContext* context) {

  CodeExecutor codeExecutor(context);

  auto contributeCallback = llvm::cast<llvm::Function>(
      codeExecutor.createContributeCallbackPrototype());
  auto reduceCallback =
      llvm::cast<llvm::Function>(codeExecutor.createReduceCallbackPrototype());
  auto introduceCallback = llvm::cast<llvm::Function>(
      codeExecutor.createIntroduceCallbackPrototype());

  std::string errStr;
  llvm::EngineBuilder builder(move(codeExecutor.shipment_));

  builder.setUseOrcMCJITReplacement(true);

  builder.setErrorStr(&errStr)
      .setEngineKind(llvm::EngineKind::JIT)
      .setOptLevel(llvm::CodeGenOpt::None)
      .setVerifyModules(true);

  std::unique_ptr<llvm::ExecutionEngine> engine(builder.create());

  if (!engine) {
    llvm::outs() << errStr << "\n";
    return llvm::None;
  }

  engine->addGlobalMapping(
      contributeCallback,
      reinterpret_cast<void*>(&CodeExecutor::contributeNodeCallback));
  engine->addGlobalMapping(
      reduceCallback,
      reinterpret_cast<void*>(&CodeExecutor::reduceNodeCallback));
  engine->addGlobalMapping(
      introduceCallback,
      reinterpret_cast<void*>(&CodeExecutor::introduceNodeCallback));

  codeExecutor.setExecutor(move(engine));
  return codeExecutor;
}

CompilationUnit* CodeExecutor::getCompilationUnit() {
  return context_->getCompilationUnit();
}

ASTContext* CodeExecutor::getASTContext() { return context_->getASTContext(); }

llvm::LLVMContext& CodeExecutor::getLLVMContext() {
  return context_->getLLVMContext();
}

llvm::Module* CodeExecutor::getModule() { return shipment(); }

llvm::Constant*
CodeExecutor::lookupGlobal(FunctionDeclASTNode const* function) {
  if (!isGlobalResolved(mangleNameOf(function))) {
    setGlobalAsUnresolved(function);
    // we resolve the function on shipment.
  }
  // When the function is defined in the same module or was submitted
  // to the JIT already just return a forward declaration to it.
  return createFunctionPrototype(function);
}

Nullable<llvm::Constant*>
CodeExecutor::lookupGlobal(MetaInstantiationExprASTNode const* inst,
                           bool requiresCompleted) {
  return context_->lookupGlobal(inst, requiresCompleted)
      .map([&](auto constant) {
        auto global = llvm::cast<llvm::Function>(constant);

        if (!isGlobalResolved(global->getName())) {
          setGlobalAsUnresolved(global);
        }

        return createFunctionPrototype(global);
      });
}

Nullable<llvm::Constant*>
CodeExecutor::resolveGlobal(llvm::Function const* /*function*/) {
  // Lookups will be resolved on the next shipment anyway
  llvm_unreachable("This context doesn't support resolving of globals!");
}

Nullable<llvm::Constant*>
CodeExecutor::resolveGlobal(FunctionDeclASTNode const* /*function*/) {
  // Lookups will be resolved on the next shipment anyway
  llvm_unreachable("This context doesn't support resolving of globals!");
}

Nullable<llvm::Constant*>
CodeExecutor::resolveGlobal(MetaInstantiationExprASTNode const* /*inst*/) {
  // Lookups will be resolved on the next shipment anyway
  llvm_unreachable("This context doesn't support resolving of globals!");
}

Nullable<MetaUnitASTNode const*>
CodeExecutor::instantiate(MetaInstantiationExprASTNode const* inst,
                          bool requiresCompleted) {

  if (auto cached = getCachedInstantiationOf(inst)) {
    return *cached;
  }

  assert(!requiresCompleted &&
         "Expected the instantiated to be completed before!");

  auto metaDecl = llvm::cast<MetaDeclASTNode>(
      inst->getDecl()->getDecl()->getDeclaringNode());

  if (shouldPrintVerboseMsg(this, VerboseFlag::Instantiations)) {
    llvm::errs() << "instantiating " << stringifyInstantiation(inst) << "...\n";
    llvm::errs().flush();

    /*getCompilationUnit()->getDiagnosticEngine()->diagnose(
        Diagnostic::NoteInstantiatingMetaDecl, inst->getSourceRange(),
        inst->getDecl()->getName());*/
  }

  /// Get the prototype of the meta function or create it
  auto metafunction = codegen(metaDecl);
  if (!metafunction) {
    return {};
  }

  std::string jumpPadName = createJumpPadTo(inst, *metafunction)->getName();

  // Make the meta function available in the JIT in order to generate an
  // own removable module for the jump pad.
  if (!shipToJIT()) {
    return {};
  }

  auto invoke = reinterpret_cast<void (*)(void*)>(
      executor_->getFunctionAddress(jumpPadName));

  auto context = getASTContext();

  ASTLayoutWriter writer;
  NodeContributor contributor(writer, getCompilationUnit(), inst, context);

  {
    /// Scope write the meta unit
    auto scope = writer.scopedWrite(context->allocate<MetaUnitASTNode>(inst));

    /// Finally invoke the
    invoke(&contributor);
  }

  // TODO maybe unload the jump pad

  auto layout = std::move(writer).buildLayout();

  if (shouldPrintVerboseMsg(this, VerboseFlag::InstantiatedLayout)) {
    dumpLayout(llvm::errs(), layout);
  }

  ASTLayoutReader reader(getCompilationUnit(), getASTContext(), layout);

  auto unit = reader.consumeMetaUnit();

  if (!canContinue()) {
    return {};
  }

  SemaAnalysis semaAnalysis(getCompilationUnit(), unit);
  semaAnalysis.checkAST();

  if (!canContinue()) {
    return {};
  }

  if (shouldPrintVerboseMsg(this, VerboseFlag::InstantiatedAST)) {
    dumpAST(llvm::errs(), unit);
  }

  // Finally cache the instantiation for further usage
  cacheInstantiationOf(inst, unit);
  return unit;
}

llvm::Module* CodeExecutor::shipment() {
  if (!shipment_) {
    shipment_ = createModule(this, "shipment");
    initializeModule(shipment_.get());
  }
  return shipment_.get();
}

void CodeExecutor::initializeModule(llvm::Module* module) {
  // Set the data layout for the host machine
  auto machine = getCompilationUnit()->getCompilerInstance()->getHostMachine();
  module->setDataLayout(machine->createDataLayout());
  module->setTargetTriple(machine->getTargetTriple().getTriple());
}

bool CodeExecutor::shipToJIT() {
  assert(shipment_ && "The shipment should contain anything!");

  // Resolves all dependencies which are required for a successful shipment
  if (!resolveDependencies()) {
    return false;
  }

  if (shouldPrintVerboseMsg(this, VerboseFlag::Shipments)) {
    // llvm::errs() << "\n\n";
    // llvm::errs() << "=============== Shipping: ================\n";
    shipment()->dump();
    // llvm::errs() << "==========================================\n\n";
    llvm::errs() << "\n\n";
    llvm::errs().flush();
  }

  // Finally pass the shipment to the executor
  executor_->addModule(std::move(shipment_));
  return true;
}

bool CodeExecutor::isGlobalResolved(llvm::StringRef symbol) const {
  return availableSymbols_.find(symbol) != availableSymbols_.end();
}

void CodeExecutor::setGlobalAsResolved(std::string symbol) {
  assert(!isGlobalResolved(symbol) && "The symbol shouldn't be available yet!");
  availableSymbols_.insert(std::move(symbol));
}

void CodeExecutor::setGlobalAsUnresolved(
    decltype(unresolvedDependencies_)::value_type entity) {
  unresolvedDependencies_.insert(entity);
}

bool CodeExecutor::resolveDependencies() {
  while (!unresolvedDependencies_.empty()) {
    auto current = *unresolvedDependencies_.begin();
    unresolvedDependencies_.erase(current);

    if (auto dep = current.dyn_cast<llvm::Function*>()) {
      if (auto resolved = context_->resolveGlobal(dep)) {
        pull(llvm::cast<llvm::Function>(*resolved));
        continue;
      } else {
        return false;
      }
    }

    if (auto dep = current.dyn_cast<FunctionDeclASTNode const*>()) {
      if (auto resolved = context_->resolveGlobal(dep)) {
        pull(llvm::cast<llvm::Function>(*resolved));
        continue;
      } else {
        return false;
      }
    }

    if (auto dep = current.dyn_cast<MetaInstantiationExprASTNode const*>()) {
      if (auto resolved = context_->resolveGlobal(dep)) {
        pull(llvm::cast<llvm::Function>(*resolved));
        continue;
      } else {
        return false;
      }
    }

    llvm_unreachable("Unknown dependency type!");
  }
  return true;
}

Nullable<llvm::Function*> CodeExecutor::codegen(MetaDeclASTNode const* node) {
  auto metaFunctionName = mangleNameOf(node);

  if (isGlobalResolved(metaFunctionName)) {
    // The symbol is available already, just return a prototype
    return llvm::cast<llvm::Function>(createFunctionPrototype(node));
  }

  // Make sure all depending meta instantiations are instantiated
  consumeMetaInstantiationsOf(node,
                              [&](MetaInstantiationExprASTNode const* inst) {
                                return bool(instantiate(inst));
                              });

  if (!canContinue()) {
    return {};
  }

  auto function = llvm::cast<llvm::Function>(createFunctionPrototype(node));

  // It's possible that the decl was resolved already
  if (isGlobalResolved(metaFunctionName)) {
    return function;
  }

  // The metafunction for the given meta decl wasn't generated yet,
  // so create it first and include it in the current shipment.
  setGlobalAsResolved(metaFunctionName);

  MetaCodegen codegen(this, function);
  codegen.codegen(node);
  return function;
}

llvm::Function* CodeExecutor::pull(llvm::Function* function) {
  assert(!function->isDeclaration() && "Expected a fully generated function!");

  std::string name = function->getName();

  // Set the function as available to avoid self dependency additions
  setGlobalAsResolved(function->getName());

  // Dependencies of the function are written into the given required set when
  // those aren't available in the JIT to avoid generation deadlocks,
  // with strong cyclic call hierarchies.
  auto materializer =
      makeValueMaterializer([this](llvm::Value* value) -> llvm::Value* {

        // We only should encounter function declarations since we
        // aren't generating any other GlobalValues
        if (auto function = llvm::dyn_cast<llvm::Function>(value)) {

          if (!isGlobalResolved(function->getName())) {
            // Register the function as dependency which must be
            // pulled from the amalgamation as well.
            setGlobalAsUnresolved(function);
          }

          // Forward declare the function for usage
          return cloneFunctionPrototype(function);
        }

        // The llvm::Mapper will do his best to resolve this
        return nullptr;
      });

  return cloneFunctionIntoModule(function, shipment(), &materializer);
}

llvm::Constant* CodeExecutor::cloneFunctionPrototype(llvm::Function* function) {
  return shipment()->getOrInsertFunction(function->getName(),
                                         function->getFunctionType());
}

Nullable<MetaUnitASTNode const*> CodeExecutor::getCachedInstantiationOf(
    MetaInstantiationExprASTNode const* inst) {
  auto previous = instantiations_.find(inst);
  if (previous != instantiations_.end()) {
    return previous->second;
  } else {
    return {};
  }
}

void CodeExecutor::cacheInstantiationOf(
    MetaInstantiationExprASTNode const* inst, MetaUnitASTNode const* unit) {
  assert(instantiations_.find(inst) == instantiations_.end() &&
         "Expected that there was no instantiations before!");
  instantiations_.insert(std::make_pair(inst, unit));
}

llvm::Function*
CodeExecutor::createJumpPadTo(MetaInstantiationExprASTNode const* inst,
                              llvm::Constant* metafunction) {
  // As of now we don't need to cache the jump pads because those are
  // theoretically only generated once.
  auto name = fmt::format("jumppad_{}_{}", inst->getDecl()->getName(),
                          mangleNameOf(inst));

  // The JumpPad has the following declaration:
  // void @@jump_???@@(void* context) {
  //   // args are generated here
  //   meta_function(args...);
  // }
  //
  // Which makes it possible to invoke the jump pad with the context only
  auto type =
      llvm::FunctionType::get(getTypeOfVoid(), {getTypeOfContextPtr()}, false);

  auto function = createFunction(name, type);

  // Codegen the jumppad
  MetaCodegen codegen(this, function);
  codegen.codegenJumpPad(metafunction, inst->getArguments());

  return function;
}

void CodeExecutor::contributeNodeCallback(void* context, void* node) {
  auto contributor = static_cast<NodeContributor*>(context);
  contributor->contribute(static_cast<ASTNode*>(node));
}

void CodeExecutor::reduceNodeCallback(void* context) {
  auto contributor = static_cast<NodeContributor*>(context);
  contributor->reduce();
}

void CodeExecutor::introduceNodeCallback(void* context, void* node, void* value,
                                         unsigned depth) {
  auto contributor = static_cast<NodeContributor*>(context);
  auto intValue = *static_cast<int*>(value);
  ASTCursor cursor(static_cast<DepthLevel>(depth));
  contributor->introduce(static_cast<ASTNode*>(node), intValue, cursor);
}
