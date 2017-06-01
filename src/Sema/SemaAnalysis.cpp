
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

#include "SemaAnalysis.hpp"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Casting.h"

#include "AST.hpp"
#include "ASTPredicate.hpp"
#include "CompilationUnit.hpp"
#include "DiagnosticEngine.hpp"

DiagnosticEngine* SemaAnalysis::diagnosticEngine() const {
  return compilationUnit_->getDiagnosticEngine();
}

void SemaAnalysis::checkAST() { accept(base_); }

void SemaAnalysis::visit(CallOperatorExprASTNode const* node) {

  if (auto declRef = llvm::dyn_cast<DeclRefExprASTNode>(node->getCallee())) {
    if (declRef->isResolved()) {
      auto callee = declRef->getDecl()->getDeclaringNode();
      if (auto function = llvm::dyn_cast<FunctionDeclASTNode>(callee)) {

        // Don't excpect any result of void returning functions
        if (!function->getReturnType() &&
            !llvm::isa<ExpressionStmtASTNode>(behind(1))) {
          diagnosticEngine()->diagnose(Diagnostic::ErrorFunctionCallReturnsVoid,
                                       declRef->getName(), declRef->getName());
          diagnosticEngine()->diagnose(Diagnostic::NoteDeclarationHint,
                                       function->getName(),
                                       function->getName());
        }

        // Compare the arguments count we are passing with the signature
        auto actual = node->getExpressions().size();
        auto expected = function->getArgDeclList()->children().size();
        if (actual != expected) {
          diagnosticEngine()->diagnose(
              Diagnostic::ErrorFunctionCallArgCountMismatch, declRef->getName(),
              declRef->getName(), actual, expected);
          diagnosticEngine()->diagnose(Diagnostic::NoteDeclarationHint,
                                       function->getName(),
                                       function->getName());
        }

      } else if (!llvm::isa<MetaDeclASTNode>(node->getCallee())) {
        // We can only call functions
        traverseNodeExpecting(
            callee, pred::isNamedDeclContext(), [&](auto promoted) {
              diagnosticEngine()->diagnose(
                  Diagnostic::ErrorCanOnlyCallFunctions, declRef->getName());
              diagnosticEngine()->diagnose(Diagnostic::NoteDeclarationHint,
                                           promoted->getName(),
                                           promoted->getName());
            });
      }
    }
  }

  return visitChildren(node);
}

void SemaAnalysis::visit(IfStmtASTNode const* node) {

  // Warn about 'if i = 0' { instead of 'if i == 0'
  if (auto binOp =
          llvm::dyn_cast<BinaryOperatorExprASTNode>(node->getExpression())) {
    if (binOp->getBinaryOperator() == ExprBinaryOperator::OperatorAssign) {
      auto range = binOp->getBinaryOperator().getAnnotation();
      diagnosticEngine()->diagnose(Diagnostic::WarningDidYouMeanEquals, range);
    }
  }

  return visitChildren(node);
}

/// Returns true when the identifier name is a reserved one
static bool isIdentifierReserved(Identifier const& identifier) {
  return llvm::StringSwitch<bool>(*identifier).Case("int", true).Default(false);
}

void SemaAnalysis::visit(FunctionDeclASTNode const* node) {
  if (isIdentifierReserved(node->getName())) {
    diagnosticEngine()->diagnose(Diagnostic::ErrorFunctionNameReserved,
                                 node->getName(), node->getName());
  }

  return visitChildren(node);
}

void SemaAnalysis::visit(MetaDeclASTNode const* node) {
  if (isIdentifierReserved(node->getName())) {
    diagnosticEngine()->diagnose(Diagnostic::ErrorMetaNameReserved,
                                 node->getName(), node->getName());
  }

  return visitChildren(node);
}

void SemaAnalysis::visit(MetaInstantiationExprASTNode const* node) {
  // For simplification we require the meta args to be int literals
  for (auto expr : node->getArguments()) {
    if (!llvm::isa<IntegerLiteralExprASTNode>(expr)) {
      diagnosticEngine()->diagnose(Diagnostic::ErrorIntegralForMetaDecl,
                                   node->getSourceRange());
    }
  }

  auto metaDecl = llvm::dyn_cast<MetaDeclASTNode>(
      node->getDecl()->getDecl()->getDeclaringNode());
  // Check whether a meta instantiation was applied to a meta decl
  if (!metaDecl) {
    diagnosticEngine()->diagnose(Diagnostic::ErrorInstantiatedNonMetaDecl,
                                 node->getSourceRange(),
                                 node->getDecl()->getName());

    diagnosticEngine()->diagnose(Diagnostic::NoteDeclarationHint,
                                 metaDecl->getName(), metaDecl->getName());
  }
  // Check whether the meta instantiation was instantiated with the correct
  // amount of parameters
  else if (metaDecl->getArgDeclList()->children().size() !=
           node->getArguments().size()) {
    auto expected = metaDecl->getArgDeclList()->children().size();
    auto actual = node->getArguments().size();

    diagnosticEngine()->diagnose(
        Diagnostic::ErrorInstantiationArgCountMismatch, node->getSourceRange(),
        node->getDecl()->getDecl()->getName(), actual, expected);

    diagnosticEngine()->diagnose(Diagnostic::NoteDeclarationHint,
                                 metaDecl->getName(), metaDecl->getName());
  }

  return visitChildren(node);
}

void SemaAnalysis::accept(ASTNode const* node) {
  depth_.push_back(node);
  ASTVisitor<>::accept(node);
  depth_.pop_back();
}

ASTNode const* SemaAnalysis::behind(std::size_t back) {
  return depth_[depth_.size() - (back + 1)];
}
