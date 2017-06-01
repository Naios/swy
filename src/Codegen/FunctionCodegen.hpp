
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

#ifndef FUNCTION_CODEGEN_HPP_INCLUDED__
#define FUNCTION_CODEGEN_HPP_INCLUDED__

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopedHashTable.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include "CodegenBase.hpp"
#include "IRContext.hpp"
#include "Nullable.hpp"

namespace llvm {
class Module;
class LLVMContext;
class Function;
class FunctionType;
class Type;
class BasicBlock;
class Value;
}

class ASTContext;
class ASTNode;
class CompilationUnit;
class CompilationUnitASTNode;
class FunctionDeclASTNode;
class AnonymousArgumentDeclASTNode;
class NamedDeclContext;
class MetaInstantiationExprASTNode;
class DeclRefExprASTNode;
class ArgumentDeclListASTNode;
class StmtASTNode;
#define FOR_EACH_STMT_NODE(NAME) class NAME##ASTNode;
#include "AST.inl"
class ExprASTNode;
#define FOR_EACH_EXPR_NODE(NAME) class NAME##ASTNode;
#include "AST.inl"
class MetaCodegen;

/// Responsible for codegening a single function
/// Preserves a state for values that are known within the function
class FunctionCodegen : public IRContextReplication,
                        public CodegenBase<FunctionCodegen> {
  friend class MetaCodegen;

public:
  using ValueMap = llvm::ScopedHashTable<NamedDeclContext const*, llvm::Value*>;

private:
  llvm::Function* function_;

  llvm::IRBuilder<> builder_;
  ValueMap values_;

public:
  FunctionCodegen(IRContext* context, llvm::Function* function);

  /// Codegens the function body
  void codegen(FunctionDeclASTNode const* node);

  llvm::BasicBlock* createEntryBlock();

  llvm::BasicBlock* setUpStackFrame(ArgumentDeclListASTNode const* args,
                                    bool isReadOnly = false,
                                    bool isThisCall = false);

  /// Introduces the value as known in the current scope
  void introduce(NamedDeclContext const* decl, llvm::Value* value) {
    values_.insert(decl, value);
  }
  /// Looks up the given NamedDeclContext and finds its associated Value
  /// in the local context. The value is never expected to be null!
  llvm::Value* lookupLocal(NamedDeclContext const* decl);

  /// Returns the llvm::Function for which the
  /// FunctionCodegen is responsible for
  llvm::Function* getFunction() const { return function_; }

  /// Creates a llvm::BasicBlock
  llvm::BasicBlock* createBlock(llvm::StringRef name);
  /// Creates a llvm::BasicBlock before the given block
  llvm::BasicBlock* createBlockBefore(llvm::BasicBlock* block,
                                      llvm::StringRef name);
  /// Creates a llvm::BasicBlock after the given block and jumps to it
  llvm::BasicBlock* createRegionAfter(llvm::BasicBlock* block,
                                      llvm::StringRef name);

  /// Codegens a statement, returns the current basic block when
  /// the scope can be continued (it wasn't terminated).
  Nullable<llvm::BasicBlock*> codegenStmt(llvm::BasicBlock* block,
                                          StmtASTNode const* stmt);
#define FOR_EACH_STMT_NODE(NAME)                                               \
  Nullable<llvm::BasicBlock*> codegenStmt(llvm::BasicBlock* block,             \
                                          NAME##ASTNode const* stmt);
#include "AST.inl"

  llvm::Value* codegenExpr(ExprASTNode const* expr);
#define FOR_EACH_EXPR_NODE(NAME)                                               \
  llvm::Value* codegenExpr(NAME##ASTNode const* expr);
#include "AST.inl"

  /// Represents a codegen supplier ref which can be used for providing
  /// a function that generates unknown statements into the given block.
  using CodegenSupplier =
      llvm::function_ref<Nullable<llvm::BasicBlock*>(llvm::BasicBlock*)>;

  /// Creates the skeleton for an if structure and invokes the CodegenSupplier
  /// at the appropriate position of statement insertion.
  Nullable<llvm::BasicBlock*>
  codegenIfStructure(llvm::BasicBlock* block, ExprASTNode const* condition,
                     CodegenSupplier codegenTrue,
                     llvm::Optional<CodegenSupplier> codegenFalse = llvm::None);

  /// Loads pointer values from memory
  llvm::Value* loadMemory(llvm::Value* value);
};

#endif // #ifndef FUNCTION_CODEGEN_HPP_INCLUDED__
