
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

#ifndef SEMA_ANALYSIS_HPP_INCLUDED__
#define SEMA_ANALYSIS_HPP_INCLUDED__

#include <vector>

#include "ASTVisitor.hpp"

class ASTNode;
class CallOperatorExprASTNode;
class CompilationUnit;
class DiagnosticEngine;

/// Offers methods to check the AST for semantical correctness
class SemaAnalysis : public ASTVisitor<> {

  CompilationUnit* compilationUnit_;
  ASTNode const* base_;
  std::vector<ASTNode const*> depth_;

public:
  SemaAnalysis(CompilationUnit* compilationUnit, ASTNode const* base)
      : compilationUnit_(compilationUnit), base_(base) {}

  /// Checks the AST for semantical correctness
  void checkAST();

  /// Returns the DiagnosticEngine which is used
  /// to emit messages about the source code
  DiagnosticEngine* diagnosticEngine() const;

  void visit(FunctionDeclASTNode const* node) override;

  void visit(MetaDeclASTNode const* node) override;

  void visit(CallOperatorExprASTNode const* node) override;

  void visit(IfStmtASTNode const* node) override;

  void visit(MetaInstantiationExprASTNode const* node) override;

  void accept(ASTNode const* node) override;

  /// Returns the nodes `back` steps behind the current node which
  /// means 0 returns the current nodes and 1 the node 1 behind.
  ASTNode const* behind(std::size_t back);
};

#endif // #ifndef SEMA_ANALYSIS_HPP_INCLUDED__
