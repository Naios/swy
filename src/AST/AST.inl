
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

#ifndef FOR_EACH_AST_NODE
  #define FOR_EACH_AST_NODE(NAME)
#endif

#ifndef FOR_EACH_STMT_NODE
  #define FOR_EACH_STMT_NODE(NAME) FOR_EACH_AST_NODE(NAME)
#endif

#ifndef FOR_EACH_EXPR_NODE
  #define FOR_EACH_EXPR_NODE(NAME) FOR_EACH_AST_NODE(NAME)
#endif

FOR_EACH_AST_NODE(CompilationUnit)
FOR_EACH_AST_NODE(MetaUnit)
FOR_EACH_AST_NODE(FunctionDecl)
FOR_EACH_AST_NODE(MetaDecl)
FOR_EACH_AST_NODE(MetaContribution)
FOR_EACH_AST_NODE(ArgumentDeclList)
FOR_EACH_AST_NODE(AnonymousArgumentDecl)
FOR_EACH_AST_NODE(NamedArgumentDecl)
FOR_EACH_AST_NODE(GlobalConstantDecl)

FOR_EACH_STMT_NODE(UnscopedCompoundStmt)
FOR_EACH_STMT_NODE(CompoundStmt)
FOR_EACH_STMT_NODE(ReturnStmt)
FOR_EACH_STMT_NODE(ExpressionStmt)
FOR_EACH_STMT_NODE(DeclStmt)
FOR_EACH_STMT_NODE(IfStmt)
FOR_EACH_STMT_NODE(MetaIfStmt)
FOR_EACH_STMT_NODE(MetaCalculationStmt)

FOR_EACH_EXPR_NODE(DeclRefExpr)
FOR_EACH_EXPR_NODE(IntegerLiteralExpr)
FOR_EACH_EXPR_NODE(BooleanLiteralExpr)
FOR_EACH_EXPR_NODE(ErroneousExpr)
FOR_EACH_EXPR_NODE(BinaryOperatorExpr)
FOR_EACH_EXPR_NODE(CallOperatorExpr)
FOR_EACH_EXPR_NODE(MetaInstantiationExpr)

#undef FOR_EACH_EXPR_NODE
#undef FOR_EACH_STMT_NODE
#undef FOR_EACH_AST_NODE


#ifndef EXPR_BINARY_OPERATOR
  #define EXPR_BINARY_OPERATOR(NAME, REP, PRECEDENCE)
#endif

EXPR_BINARY_OPERATOR(Mul, "*", 50)
EXPR_BINARY_OPERATOR(Div, "/", 50)
EXPR_BINARY_OPERATOR(Plus, "+", 40)
EXPR_BINARY_OPERATOR(Minus, "-", 40)
EXPR_BINARY_OPERATOR(LessThan, "<", 30)
EXPR_BINARY_OPERATOR(GreaterThan, ">", 30)
EXPR_BINARY_OPERATOR(LessThanOrEq, "<=", 30)
EXPR_BINARY_OPERATOR(GreaterThanOrEq, ">=", 30)
EXPR_BINARY_OPERATOR(Equal, "==", 20)
EXPR_BINARY_OPERATOR(NotEqual, "!=", 20)
EXPR_BINARY_OPERATOR(Assign, "=", 10)

#undef EXPR_BINARY_OPERATOR
