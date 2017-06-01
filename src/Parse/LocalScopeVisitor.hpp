
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

#ifndef LOCAL_SCOPE_VISITOR_HPP_INCLUDED__
#define LOCAL_SCOPE_VISITOR_HPP_INCLUDED__

#include "ASTLayout.hpp"
#include "ASTPredicate.hpp"
#include "BasicTreeVisitor.hpp"

class LocalScopeVisitor : public BasicTreeVisitor {
  ASTLayoutWriter writer_;

public:
  LocalScopeVisitor(CompilationUnit* compilationUnit, ASTContext* astContextRef)
      : BasicTreeVisitor(compilationUnit, astContextRef) {}

  ASTLayout buildLayout() && { return std::move(writer_).buildLayout(); }

  antlrcpp::Any visitCompilationUnit(
      GeneratedParser::CompilationUnitContext* context) override;

  antlrcpp::Any
  visitFunctionDecl(GeneratedParser::FunctionDeclContext* context) override;

  antlrcpp::Any visitArgumentDeclList(
      GeneratedParser::ArgumentDeclListContext* context) override;

  antlrcpp::Any
  visitArgumentDecl(GeneratedParser::ArgumentDeclContext* context) override;

  antlrcpp::Any
  visitMetaDecl(GeneratedParser::MetaDeclContext* context) override;

  antlrcpp::Any visitMetaInstantiationExpr(
      GeneratedParser::MetaInstantiationExprContext* context) override;

  antlrcpp::Any visitMetaContribution(
      GeneratedParser::MetaContributionContext* context) override;

  antlrcpp::Any
  visitDeclRefExpr(GeneratedParser::DeclRefExprContext* context) override;

  antlrcpp::Any visitIntegerLiteralExpr(
      GeneratedParser::IntegerLiteralExprContext* context) override;

  antlrcpp::Any visitBooleanLiteralExpr(
      GeneratedParser::BooleanLiteralExprContext* context) override;

  antlrcpp::Any
  visitReturnStmt(GeneratedParser::ReturnStmtContext* context) override;

  antlrcpp::Any
  visitCompoundStmt(GeneratedParser::CompoundStmtContext* context) override;

  antlrcpp::Any visitUnscopedCompoundStmt(
      GeneratedParser::UnscopedCompoundStmtContext* context) override;

  antlrcpp::Any visitExpr(GeneratedParser::ExprContext* context) override;

  antlrcpp::Any
  visitExprStmt(GeneratedParser::ExprStmtContext* context) override;

  antlrcpp::Any
  visitDeclStmt(GeneratedParser::DeclStmtContext* context) override;

  antlrcpp::Any visitIfStmt(GeneratedParser::IfStmtContext* context) override;

  antlrcpp::Any
  visitMetaIfStmt(GeneratedParser::MetaIfStmtContext* context) override;

  antlrcpp::Any visitMetaCalculationStmt(
      GeneratedParser::MetaCalculationStmtContext* context) override;

  antlrcpp::Any visitCallOperatorExpr(
      GeneratedParser::CallOperatorExprContext* context) override;

  /// Contributes a node from the given context.
  template <typename T, typename... Args>
  std::nullptr_t contributeFrom(antlr4::tree::ParseTree* context,
                                Args&&... args) {
    auto node = allocate<T>(std::forward<Args>(args)...);
    contributeFromImpl(pred::hasChildren()(node),
                       decltype(pred::isRequiringReduceMarker()(node))::value,
                       context, node);
    return nullptr;
  }

private:
  RangeAnnotated<ExprBinaryOperator>
  getBinaryOperatorOf(GeneratedParser::BinaryOperatorContext* context);

  void contributeFromImpl(std::false_type, bool /*insertReduceMarker*/,
                          antlr4::tree::ParseTree* /*context*/, ASTNode* node) {
    writer_.directWrite(node);
  }

  void contributeFromImpl(std::true_type, bool insertReduceMarker,
                          antlr4::tree::ParseTree* context, ASTNode* node);
};

#endif // #ifndef LOCAL_SCOPE_VISITOR_HPP_INCLUDED__
