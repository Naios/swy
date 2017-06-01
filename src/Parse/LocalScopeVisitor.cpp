
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

#include "LocalScopeVisitor.hpp"

#include <tuple>

#include "llvm/Support/ErrorHandling.h"

#include "AST.hpp"
#include "DiagnosticEngine.hpp"

antlrcpp::Any LocalScopeVisitor::visitCompilationUnit(
    GeneratedParser::CompilationUnitContext* context) {

  return contributeFrom<CompilationUnitASTNode>(context);
}

antlrcpp::Any LocalScopeVisitor::visitFunctionDecl(
    GeneratedParser::FunctionDeclContext* context) {

  auto name = identifierOf(context->Identifier());
  return contributeFrom<FunctionDeclASTNode>(context, name);
}

antlrcpp::Any LocalScopeVisitor::visitArgumentDeclList(
    GeneratedParser::ArgumentDeclListContext* context) {
  return contributeFrom<ArgumentDeclListASTNode>(context);
}

antlrcpp::Any LocalScopeVisitor::visitArgumentDecl(
    GeneratedParser::ArgumentDeclContext* context) {

  if (auto nameContext = context->argumentName()) {
    auto name = identifierOf(nameContext->Identifier());
    return contributeFrom<NamedArgumentDeclASTNode>(context, name);
  } else {
    return contributeFrom<AnonymousArgumentDeclASTNode>(context);
  }
}

antlrcpp::Any
LocalScopeVisitor::visitMetaDecl(GeneratedParser::MetaDeclContext* context) {

  auto name = identifierOf(context->Identifier());
  return contributeFrom<MetaDeclASTNode>(context, name);
}

antlrcpp::Any LocalScopeVisitor::visitMetaInstantiationExpr(
    GeneratedParser::MetaInstantiationExprContext* context) {

  auto range = sourceRangeOf(context->OperatorLessThan(),
                             context->OperatorGreaterThan());

  return contributeFrom<MetaInstantiationExprASTNode>(context, range);
}

antlrcpp::Any LocalScopeVisitor::visitMetaContribution(
    GeneratedParser::MetaContributionContext* context) {

  auto range = sourceRangeOf(context->OpenCurly(), context->CloseCurly());
  return contributeFrom<MetaContributionASTNode>(context, range);
}

antlrcpp::Any LocalScopeVisitor::visitDeclRefExpr(
    GeneratedParser::DeclRefExprContext* context) {

  auto name = identifierOf(context->Identifier());
  return contributeFrom<DeclRefExprASTNode>(context, name);
}

antlrcpp::Any LocalScopeVisitor::visitIntegerLiteralExpr(
    GeneratedParser::IntegerLiteralExprContext* context) {
  auto rep = identifierOf(context->IntegerLiteral());

  std::int32_t value;
  if (rep->getAsInteger(10U, value)) {
    diagnosticEngine()->diagnose(Diagnostic::ErrorConvertionFailure, rep, rep);

    return contributeFrom<ErroneousExprASTNode>(context);
  }

  return contributeFrom<IntegerLiteralExprASTNode>(
      context, annotate(value, rep.getAnnotation()));
}

antlrcpp::Any LocalScopeVisitor::visitBooleanLiteralExpr(
    GeneratedParser::BooleanLiteralExprContext* context) {

  auto rep = [&] {
    if (context->True())
      return std::make_tuple(true, identifierOf(context->True()));
    else
      return std::make_tuple(false, identifierOf(context->False()));
  }();

  auto annotated =
      annotate(std::get<bool>(rep), std::get<Identifier>(rep).getAnnotation());

  return contributeFrom<BooleanLiteralExprASTNode>(context, annotated);
}

antlrcpp::Any LocalScopeVisitor::visitReturnStmt(
    GeneratedParser::ReturnStmtContext* context) {

  return contributeFrom<ReturnStmtASTNode>(context);
}

antlrcpp::Any LocalScopeVisitor::visitCompoundStmt(
    GeneratedParser::CompoundStmtContext* context) {

  return contributeFrom<CompoundStmtASTNode>(context);
}

antlrcpp::Any LocalScopeVisitor::visitUnscopedCompoundStmt(
    GeneratedParser::UnscopedCompoundStmtContext* context) {

  return contributeFrom<UnscopedCompoundStmtASTNode>(context);
}

RangeAnnotated<ExprBinaryOperator> LocalScopeVisitor::getBinaryOperatorOf(
    GeneratedParser::BinaryOperatorContext* context) {
#define EXPR_BINARY_OPERATOR(NAME, REP, ...)                                   \
  if (context->Operator##NAME())                                               \
    return annotate(ExprBinaryOperator::Operator##NAME,                        \
                    sourceRangeOf(context->Operator##NAME()));
#include "AST.inl"
  llvm_unreachable("The binaray operator is unknown!");
}

antlrcpp::Any
LocalScopeVisitor::visitExpr(GeneratedParser::ExprContext* context) {

  if (context->binaryOperator()) {
    auto binaryOperator = getBinaryOperatorOf(context->binaryOperator());
    return contributeFrom<BinaryOperatorExprASTNode>(context, binaryOperator);
  }
  return visitChildren(context);
}

antlrcpp::Any
LocalScopeVisitor::visitExprStmt(GeneratedParser::ExprStmtContext* context) {
  return contributeFrom<ExpressionStmtASTNode>(context);
}

antlrcpp::Any
LocalScopeVisitor::visitDeclStmt(GeneratedParser::DeclStmtContext* context) {

  auto type = identifierOf(context->varDecl()->varDeclType()->Identifier());
  auto name = identifierOf(context->varDecl()->varDeclName()->Identifier());

  if (type != "int") {
    diagnosticEngine()->diagnose(Diagnostic::ErrorOnlyIntPermitted, type, type);
  }

  return contributeFrom<DeclStmtASTNode>(context, name);
}

antlrcpp::Any
LocalScopeVisitor::visitIfStmt(GeneratedParser::IfStmtContext* context) {

  return contributeFrom<IfStmtASTNode>(context);
}

antlrcpp::Any LocalScopeVisitor::visitMetaIfStmt(
    GeneratedParser::MetaIfStmtContext* context) {

  return contributeFrom<MetaIfStmtASTNode>(context);
}

antlrcpp::Any LocalScopeVisitor::visitMetaCalculationStmt(
    GeneratedParser::MetaCalculationStmtContext* context) {

  return contributeFrom<MetaCalculationStmtASTNode>(context);
}

antlrcpp::Any LocalScopeVisitor::visitCallOperatorExpr(
    GeneratedParser::CallOperatorExprContext* context) {

  return contributeFrom<CallOperatorExprASTNode>(context);
}

void LocalScopeVisitor::contributeFromImpl(std::true_type,
                                           bool insertReduceMarker,
                                           antlr4::tree::ParseTree* context,
                                           ASTNode* node) {
  writer_.directWrite(node);
  visitChildren(context);
  if (insertReduceMarker) {
    writer_.markReduce();
  }
}
