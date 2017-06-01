
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

#include "AST.hpp"

#include "ASTPredicate.hpp"
#include "ASTTraversal.hpp"

bool NamedDeclContext::isFunctionDecl() const {
  return llvm::isa<FunctionDeclASTNode>(getDeclaringNode());
}

bool NamedDeclContext::isVarDecl() const {
  auto node = getDeclaringNode();
  return llvm::isa<NamedArgumentDeclASTNode>(node) ||
         llvm::isa<DeclStmtASTNode>(node);
}

/// Returns true when the declaration is a meta decl
bool NamedDeclContext::isMetaDecl() const {
  return llvm::isa<MetaDeclASTNode>(getDeclaringNode());
}

bool NamedDeclContext::isGlobalConstant() const {
  return llvm::isa<GlobalConstantDeclASTNode>(getDeclaringNode());
}

llvm::SmallVector<ASTNode*, 3> FunctionDeclASTNode::children() {
  llvm::SmallVector<ASTNode*, 3> seq;
  seq.push_back(*arguments_);
  if (returnType_) {
    seq.push_back(*returnType_);
  }
  seq.push_back(body_);
  return seq;
}

llvm::SmallVector<ASTNode const*, 3> FunctionDeclASTNode::children() const {
  llvm::SmallVector<ASTNode const*, 3> seq;
  seq.push_back(*arguments_);
  if (returnType_) {
    seq.push_back(*returnType_);
  }
  seq.push_back(body_);
  return seq;
}

std::array<ExprASTNode*, 1> ExpressionStmtASTNode::children() {
  return {{*expression_}};
}

std::array<ExprASTNode const*, 1> ExpressionStmtASTNode::children() const {
  return {{*expression_}};
}

std::array<ASTNode*, 2> MetaDeclASTNode::children() {
  return {{*arguments_, *contribution_}};
}

std::array<ASTNode const*, 2> MetaDeclASTNode::children() const {
  return {{*arguments_, *contribution_}};
}

std::array<StmtASTNode*, 1> MetaCalculationStmtASTNode::children() {
  return {{*stmt_}};
}

std::array<StmtASTNode const*, 1> MetaCalculationStmtASTNode::children() const {
  return {{*stmt_}};
}

std::array<ExprASTNode*, 2> BinaryOperatorExprASTNode::children() {
  return {{*left_, *right_}};
}

std::array<ExprASTNode const*, 2> BinaryOperatorExprASTNode::children() const {
  return {{*left_, *right_}};
}

std::array<ExprASTNode*, 1> DeclStmtASTNode::children() {
  return {{*expression_}};
}

std::array<ExprASTNode const*, 1> DeclStmtASTNode::children() const {
  return {{*expression_}};
}

ASTChildSequence CallOperatorExprASTNode::children() {
  ASTChildSequence seq{*callee_};
  seq.append(expressions_.begin(), expressions_.end());
  return seq;
}

ConstASTChildSequence CallOperatorExprASTNode::children() const {
  ConstASTChildSequence seq{*callee_};
  seq.append(expressions_.begin(), expressions_.end());
  return seq;
}

llvm::SmallVector<ExprASTNode*, 4> MetaInstantiationExprASTNode::children() {
  llvm::SmallVector<ExprASTNode*, 4> seq{*decl_};
  seq.append(arguments_.begin(), arguments_.end());
  return seq;
}

llvm::SmallVector<ExprASTNode const*, 4>
MetaInstantiationExprASTNode::children() const {
  llvm::SmallVector<ExprASTNode const*, 4> seq{*decl_};
  seq.append(arguments_.begin(), arguments_.end());
  return seq;
}

bool StmtASTNode::classof(ASTNode const* node) {
  return traverseNode(node, decorate(identityOf<bool>(), pred::isStmtNode()));
}

bool ExprASTNode::classof(ASTNode const* node) {
  return traverseNode(node, decorate(identityOf<bool>(), pred::isExprNode()));
}
