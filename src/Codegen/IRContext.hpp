
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

#ifndef IRCONTEXT_INCLUDED__
#define IRCONTEXT_INCLUDED__

#include "Nullable.hpp"

namespace llvm {
class Module;
class LLVMContext;
class Constant;
class FunctionType;
class Function;
}

class ASTContext;
class CompilationUnit;
class FunctionDeclASTNode;
class MetaInstantiationExprASTNode;

/// Represents a class independent context retrieval layer
/// which provides information about the current context and module.
class IRContext {
public:
  IRContext() = default;
  virtual ~IRContext() = default;

  /// Returns the module of the context
  virtual CompilationUnit* getCompilationUnit() = 0;
  /// Returns the ASTContext
  virtual ASTContext* getASTContext() = 0;
  /// Returns the LLVM of the context
  virtual llvm::LLVMContext& getLLVMContext() = 0;
  /// Returns the module of the context
  virtual llvm::Module* getModule() = 0;

  /// Looks the given function decl up inside the current context which
  /// potentially can return an ungenerated prototype of the global.
  virtual llvm::Constant* lookupGlobal(FunctionDeclASTNode const* function) = 0;
  /// Looks the given instantiation up inside the current context which
  /// potentially can return an ungenerated prototype of the global.
  virtual Nullable<llvm::Constant*>
  lookupGlobal(MetaInstantiationExprASTNode const* inst,
               bool requiresCompleted = false) = 0;

  /// Resolves the given function inside the current context which
  /// returns a guaranteed completed global.
  virtual Nullable<llvm::Constant*>
  resolveGlobal(llvm::Function const* function) = 0;
  /// Resolves the given function decl inside the current context which
  /// returns a guaranteed completed global.
  virtual Nullable<llvm::Constant*>
  resolveGlobal(FunctionDeclASTNode const* function) = 0;
  /// Resolves the given instantiation inside the current context which
  /// returns a guaranteed completed global.
  virtual Nullable<llvm::Constant*>
  resolveGlobal(MetaInstantiationExprASTNode const* inst) = 0;
};

/// Replicates the underlaying IRContext and makes it's methods to members
class IRContextReplication {
  IRContext* context_;

public:
  explicit IRContextReplication(IRContext* context) : context_(context) {}

  IRContext* getIRContext() { return context_; }
  CompilationUnit* getCompilationUnit() {
    return context_->getCompilationUnit();
  }
  ASTContext* getASTContext() { return context_->getASTContext(); }
  llvm::LLVMContext& getLLVMContext() { return context_->getLLVMContext(); }
  llvm::Module* getModule() { return context_->getModule(); }
  llvm::Constant* lookupGlobal(FunctionDeclASTNode const* function) {
    return context_->lookupGlobal(function);
  }
  Nullable<llvm::Constant*>
  lookupGlobal(MetaInstantiationExprASTNode const* inst,
               bool requiresCompleted = false) {
    return context_->lookupGlobal(inst, requiresCompleted);
  }
  Nullable<llvm::Constant*> resolveGlobal(llvm::Function const* function) {
    return context_->resolveGlobal(function);
  }
  Nullable<llvm::Constant*> resolveGlobal(FunctionDeclASTNode const* function) {
    return context_->resolveGlobal(function);
  }
  Nullable<llvm::Constant*>
  resolveGlobal(MetaInstantiationExprASTNode const* inst) {
    return context_->resolveGlobal(inst);
  }
};

#endif // #ifndef IRCONTEXT_INCLUDED__
