
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

#include "ASTCloner.hpp"

#include "llvm/Support/Casting.h"

#include "AST.hpp"
#include "ASTTraversal.hpp"

ASTNode* ASTCloner::clone(ASTNode const* node) {
  switch (node->getKind()) {
#define FOR_EACH_AST_NODE(NAME)                                                \
  case ASTKind::Kind##NAME:                                                    \
    return clone##NAME(llvm::cast<NAME##ASTNode>(node));
#include "AST.inl"
    default:
      llvm_unreachable("Unknown node type!");
  }
}

CompilationUnitASTNode*
ASTCloner::cloneCompilationUnit(CompilationUnitASTNode const* /*node*/) {
  return allocate<CompilationUnitASTNode>();
}

MetaUnitASTNode* ASTCloner::cloneMetaUnit(MetaUnitASTNode const* node) {
  return allocate<MetaUnitASTNode>(node->getInstantiation());
}

FunctionDeclASTNode*
ASTCloner::cloneFunctionDecl(FunctionDeclASTNode const* node) {
  return allocate<FunctionDeclASTNode>(relocate(node->getName()));
}

MetaDeclASTNode* ASTCloner::cloneMetaDecl(MetaDeclASTNode const* node) {
  return allocate<MetaDeclASTNode>(node->getName());
}

GlobalConstantDeclASTNode*
ASTCloner::cloneGlobalConstantDecl(GlobalConstantDeclASTNode const* node) {
  return allocate<GlobalConstantDeclASTNode>(node->getName());
}

MetaContributionASTNode*
ASTCloner::cloneMetaContribution(MetaContributionASTNode const* node) {
  return allocate<MetaContributionASTNode>(relocate(node->getSourceRange()));
}

MetaInstantiationExprASTNode* ASTCloner::cloneMetaInstantiationExpr(
    MetaInstantiationExprASTNode const* node) {
  return allocate<MetaInstantiationExprASTNode>(
      relocate(node->getSourceRange()));
}

ArgumentDeclListASTNode*
ASTCloner::cloneArgumentDeclList(ArgumentDeclListASTNode const* /*node*/) {
  return allocate<ArgumentDeclListASTNode>();
}

AnonymousArgumentDeclASTNode* ASTCloner::cloneAnonymousArgumentDecl(
    AnonymousArgumentDeclASTNode const* /*node*/) {
  return allocate<AnonymousArgumentDeclASTNode>();
}

NamedArgumentDeclASTNode*
ASTCloner::cloneNamedArgumentDecl(NamedArgumentDeclASTNode const* node) {
  return allocate<NamedArgumentDeclASTNode>(relocate(node->getName()));
}

UnscopedCompoundStmtASTNode* ASTCloner::cloneUnscopedCompoundStmt(
    UnscopedCompoundStmtASTNode const* /*node*/) {
  return allocate<UnscopedCompoundStmtASTNode>();
}

CompoundStmtASTNode*
ASTCloner::cloneCompoundStmt(CompoundStmtASTNode const* /*node*/) {
  return allocate<CompoundStmtASTNode>();
}

ReturnStmtASTNode*
ASTCloner::cloneReturnStmt(ReturnStmtASTNode const* /*node*/) {
  return allocate<ReturnStmtASTNode>();
}

ExpressionStmtASTNode*
ASTCloner::cloneExpressionStmt(ExpressionStmtASTNode const* /*node*/) {
  return allocate<ExpressionStmtASTNode>();
}

DeclStmtASTNode* ASTCloner::cloneDeclStmt(DeclStmtASTNode const* node) {
  return allocate<DeclStmtASTNode>(relocate(node->getName()));
}

IfStmtASTNode* ASTCloner::cloneIfStmt(IfStmtASTNode const* /*node*/) {
  return allocate<IfStmtASTNode>();
}

MetaIfStmtASTNode*
ASTCloner::cloneMetaIfStmt(MetaIfStmtASTNode const* /*node*/) {
  return allocate<MetaIfStmtASTNode>();
}

MetaCalculationStmtASTNode* ASTCloner::cloneMetaCalculationStmt(
    MetaCalculationStmtASTNode const* /*node*/) {
  return allocate<MetaCalculationStmtASTNode>();
}

DeclRefExprASTNode*
ASTCloner::cloneDeclRefExpr(DeclRefExprASTNode const* node) {
  return allocate<DeclRefExprASTNode>(relocate(node->getName()));
}

IntegerLiteralExprASTNode*
ASTCloner::cloneIntegerLiteralExpr(IntegerLiteralExprASTNode const* node) {
  return allocate<IntegerLiteralExprASTNode>(relocate(node->getLiteral()));
}

BooleanLiteralExprASTNode*
ASTCloner::cloneBooleanLiteralExpr(BooleanLiteralExprASTNode const* node) {
  return allocate<BooleanLiteralExprASTNode>(relocate(node->getLiteral()));
}

ErroneousExprASTNode*
ASTCloner::cloneErroneousExpr(ErroneousExprASTNode const* /*node*/) {
  return allocate<ErroneousExprASTNode>();
}

BinaryOperatorExprASTNode*
ASTCloner::cloneBinaryOperatorExpr(BinaryOperatorExprASTNode const* node) {
  return allocate<BinaryOperatorExprASTNode>(
      relocate(node->getBinaryOperator()));
}

CallOperatorExprASTNode*
ASTCloner::cloneCallOperatorExpr(CallOperatorExprASTNode const* /*node*/) {
  return allocate<CallOperatorExprASTNode>();
}
