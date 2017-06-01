
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

#ifndef CODEGEN_INSTANCE_HPP_INCLUDED__
#define CODEGEN_INSTANCE_HPP_INCLUDED__

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/PointerUnion.h"

#include "CodeExecutor.hpp"
#include "CodegenBase.hpp"
#include "IRContext.hpp"
#include "Nullable.hpp"
#include "ScopeLeaveAction.hpp"

namespace llvm {
class Module;
class LLVMContext;
class Function;
class FunctionType;
class Type;
class Constant;
namespace legacy {
class FunctionPassManager;
}
}

class ASTContext;
class ASTNode;
class CompilationUnit;
class FunctionDeclASTNode;
class AnonymousArgumentDeclASTNode;
class CompilationUnitASTNode;
class MetaASTEvaluator;
class CodegenInstance;

/// Provides the facility for generating LLVM IR from the AST
class CodegenInstance : public IRContext, public CodegenBase<CodegenInstance> {
  CompilationUnit* compilationUnit_;
  ASTContext* astContext_;
  std::unique_ptr<CodeExecutor> codeExecuter_;

  std::unique_ptr<llvm::LLVMContext> llvmContext_;
  std::unique_ptr<llvm::Module> amalgamation_;

  std::unique_ptr<llvm::legacy::FunctionPassManager> passManager_;

  /// Contains the dependencies which still must be generated
  std::unordered_set<ASTNode const*> dependencies_;

  /// Contains the current generated functions, useful for cycle detection
  std::unordered_set<ASTNode const*> currentGeneration_;

  std::unordered_map<llvm::Function const*,
                     llvm::PointerUnion<FunctionDeclASTNode const*,
                                        MetaInstantiationExprASTNode const*>>
      functionSource_;

  CodegenInstance(CompilationUnit* compilationUnit, ASTContext* astContext);

public:
  ~CodegenInstance();

  static std::unique_ptr<CodegenInstance>
  createFor(CompilationUnit* compilationUnit, ASTContext* astContext);

  CompilationUnit* getCompilationUnit() override;
  ASTContext* getASTContext() override { return astContext_; }
  llvm::LLVMContext& getLLVMContext() override;
  llvm::Module* getModule() override;
  llvm::Constant* lookupGlobal(FunctionDeclASTNode const* function) override;
  Nullable<llvm::Constant*>
  lookupGlobal(MetaInstantiationExprASTNode const* inst,
               bool requiresCompleted = false) override;
  Nullable<llvm::Constant*>
  resolveGlobal(llvm::Function const* function) override;
  Nullable<llvm::Constant*>
  resolveGlobal(FunctionDeclASTNode const* function) override;
  Nullable<llvm::Constant*>
  resolveGlobal(MetaInstantiationExprASTNode const* inst) override;

  /// Generates the IR for the given CompilationUnitASTNode
  bool codegen(CompilationUnitASTNode const* compilationUnitASTNode);

  /// Dumps the module to stdout
  void dump();

private:
  /// Mark an ASTNode as currently generated to detect strong cycles
  ScopeLeaveAction enterGeneration(ASTNode const* node);

  /// Codegens the body of a function
  Nullable<llvm::Function*> codegen(FunctionDeclASTNode const* node);

  Nullable<llvm::Constant*> codegen(MetaInstantiationExprASTNode const* node);

  /// Codegen dummy
  bool codegen(ASTNode const* /*node*/) { return true; }

  /// Instantiates the given MetaInstantiation and returns
  /// it's exported node on success.
  Nullable<ASTNode const*> instantiate(MetaInstantiationExprASTNode const* inst,
                                       bool requiresCompleted = false);

  /// Runs optimization passes on the function depending
  /// on the configured optimization level.
  void optimizeFunction(llvm::Function* function);
};

#endif // #ifndef CODEGEN_INSTANCE_HPP_INCLUDED__
