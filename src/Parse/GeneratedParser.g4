
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

parser grammar GeneratedParser;

options {
  tokenVocab = GeneratedLexer;
  superClass=BasicParser;
  // contextSuperClass=MyRuleNode; // ParserRuleContext
}

@parser::header {
  #include "AST.hpp"
  #include "ASTScope.hpp"
  #include "BasicParser.hpp"
}

// Actual grammar start.
compilationUnit
  : (runtimeGlobalScopeNode | metaDecl)* EOF;

runtimeGlobalScopeNode
  : functionDecl
  | { isInMetaDecl() }? metaStmt
  ;

functionDecl
  : Identifier OpenPar argumentDeclList ClosePar
    returnDecl Arrow
    { if (isInMetaDecl()) {
        assert(isInMetaDepth(MetaDepth::DepthGlobalScope)); 
        enterDepth(MetaDepth::DepthLocalScope);
      }
    }
    statement
    { if (isInMetaDecl()) {
        leaveDepth();
        assert(isInMetaDepth(MetaDepth::DepthGlobalScope)); 
      }
    };

metaDecl
  : Identifier OperatorLessThan argumentDeclList OperatorGreaterThan Arrow
    { assert(!isInMetaDecl()); enterDepth(MetaDepth::DepthGlobalScope); }
      metaContribution
    { leaveDepth(); assert(!isInMetaDecl()); }
  ;

metaContribution
  : /*(metaScopeNode | (*/ OpenCurly metaScopeNode* CloseCurly /*))*/
  ;

metaScopeNode
  : ({ isInMetaDepth(MetaDepth::DepthGlobalScope) }? runtimeGlobalScopeNode)
  | ({ isInMetaDepth(MetaDepth::DepthLocalScope)  }? statement)
  ;

metaStmt
  : metaCalculationStmt
  | metaIfStmt
  ;

metaCalculationStmt
  : { enterDepth(MetaDepth::DepthNone); }
      Meta unscopedCompoundStmt
    { leaveDepth(); }
  ;

metaIfStmt
  : Meta If metaCalculationExpr metaContribution metaElseStmt?
  ;

metaElseStmt
  : Else metaContribution
  ;

returnDecl
  : argumentDecl?
  ;

argumentDeclList
  : (argumentDecl (Comma argumentDecl)*)?
  ;

argumentDecl
  : argumentType argumentName?
  ;

argumentType
  : Identifier
  ;

argumentName
  : Identifier
  ;

statement
  : declStmt
  | exprStmt
  | returnStmt
  | ifStmt
  | compoundStmt
  | { isInMetaDecl() && !isInMetaDepth(MetaDepth::DepthNone) }? metaStmt
  ;

compoundStmt
  : basicCompoundStmt
  ;

unscopedCompoundStmt
  : basicCompoundStmt
  ;

basicCompoundStmt
  : OpenCurly statement* CloseCurly
  ;

// forStmt
//   : For forOptionalT0 compoundStmt

declStmt
  : varDecl OperatorAssign expr Semicolon
  ;

varDecl
  : varDeclType varDeclName
  ;

varDeclType
  : Identifier
  ;

varDeclName
  : Identifier
  ;

exprStmt
  : expr Semicolon
  ;

returnStmt
  : Return expr? Semicolon
  ;

ifStmt
  : If expr compoundStmt elseStmt?
  ;

elseStmt
  : Else compoundStmt
  ;

metaCalculationExpr
  : { enterDepth(MetaDepth::DepthNone); }
      expr
    { leaveDepth(); }
  ;

// exprs with precedences
expr
  : unaryExpr
  | metaInstantiationExpr
  | declRefExpr
  | OpenPar expr ClosePar
  | integerLiteralExpr
  | booleanLiteralExpr
  | expr binaryOperator expr
  ;

unaryExpr
  : enclosingExpr
  ;

enclosingExpr
  : callOperatorExpr
  ;

callOperatorExpr
  : (declRefExpr | metaInstantiationExpr) OpenPar exprList ClosePar
  ;

exprList
  : (expr (Comma expr)*)?
  ;

integerLiteralExpr
  : IntegerLiteral
  ;

booleanLiteralExpr
  : True | False
  ;

declRefExpr
  : Identifier
  ;

metaInstantiationExpr
  : declRefExpr OperatorLessThan exprList OperatorGreaterThan
  ;

// No operator precedence support
binaryOperator
  : <assoc = right>OperatorAssign
  | OperatorMul
  | OperatorDiv
  | OperatorPlus
  | OperatorMinus
  | OperatorLessThan
  | OperatorGreaterThan
  | OperatorLessThanOrEq
  | OperatorGreaterThanOrEq
  | OperatorEqual
  | OperatorNotEqual
  ;

