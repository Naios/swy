
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

#ifndef META_CODEGEN_CODEGEN_HPP_INCLUDED__
#define META_CODEGEN_CODEGEN_HPP_INCLUDED__

#include "ASTCursor.hpp"
#include "FunctionCodegen.hpp"
#include "IRContext.hpp"
#include "Nullable.hpp"

namespace llvm {
class Function;
class BasicBlock;
class Instruction;
class Value;
}

class ASTNode;
class IRContext;
class MetaDeclASTNode;
class MetaASTEvaluator;
class MetaContributionASTNode;
class MetaInstantiationExprASTNode;
class MetaIfStmtASTNode;
class MetaCalculationStmtASTNode;

/// Is responsible for codegening a single meta decl
/// Preserves a state for values that are known within the function
class MetaCodegen : public IRContextReplication,
                    public CodegenBase<MetaCodegen> {
  FunctionCodegen functionCodegen_;
  llvm::Function* function_;
  ASTCursor cursor; /// Tracks the depth level of contributed nodes

public:
  MetaCodegen(IRContext* context, llvm::Function* function);

  /// Codegens the meta function body
  void codegen(MetaDeclASTNode const* metaDecl);

  Nullable<llvm::BasicBlock*> codegenMeta(llvm::BasicBlock* block,
                                          MetaDeclASTNode const* node);
  Nullable<llvm::BasicBlock*> codegenMeta(llvm::BasicBlock* block,
                                          MetaContributionASTNode const* node);
  Nullable<llvm::BasicBlock*> codegenMeta(llvm::BasicBlock* block,
                                          MetaIfStmtASTNode const* node);
  Nullable<llvm::BasicBlock*>
  codegenMeta(llvm::BasicBlock* block, MetaCalculationStmtASTNode const* node);
  Nullable<llvm::BasicBlock*> codegenMeta(llvm::BasicBlock* block,
                                          ASTNode const* node);

  /// Codegens a jump pad function which:
  /// - passes the first argument of the function to the callee
  /// - codegens the arguments and passes it to the callee as well
  void codegenJumpPad(llvm::Constant* callee,
                      llvm::ArrayRef<ExprASTNode const*> args);

private:
  /// Codegens the children of the given node as meta contribution
  Nullable<llvm::BasicBlock*>
  codegenChildrenContribution(llvm::BasicBlock* block, ASTNode const* node);

  /// Adds the meta data string for the given action and node to the instruction
  void setMetaDataOf(llvm::Instruction* instr, llvm::StringRef action,
                     ASTNode const* node);

  /// Returns the context argument of the meta function
  llvm::Value* getContextArgument() const;
  /// Creates a call to contribute the given node to the layout
  void createContributeNode(ASTNode const* node);
  /// Creates a call to reduce the current layout
  void createReduceNode(ASTNode const* node);
  /// Creates a call to introduce the nodes value into the current layout
  void createIntroduceNode(NamedDeclContext const* decl, llvm::Value* value);
  /// Materializes a pointer constant
  llvm::Value* getPointerToNode(ASTNode const* node);
};

#endif // #ifndef META_CODEGEN_CODEGEN_HPP_INCLUDED__
