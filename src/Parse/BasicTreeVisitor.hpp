
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

#ifndef BASIC_TREE_VISITOR_HPP_INCLUDED__
#define BASIC_TREE_VISITOR_HPP_INCLUDED__

#include "GeneratedParserBaseVisitor.h"

#include "BasicTreeSupport.hpp"

class CompilationUnit;
class DiagnosticEngine;

/// Provides basic support methods for ANTLR parse tree visitors and contexts
class BasicTreeVisitor : public GeneratedParserBaseVisitor,
                         public BasicTreeSupport {
public:
  BasicTreeVisitor(CompilationUnit* compilationUnit, ASTContext* astContext)
      : BasicTreeSupport(compilationUnit, astContext) {}

  /// Returns the SourceLocation of the given TerminalNode
  SourceLocation sourceLocationOf(antlr4::tree::TerminalNode* node) const {
    return sourceLocationOf(node->getSymbol());
  }
  /// Returns the SourceLocation of the given Token
  SourceLocation sourceLocationOf(antlr4::Token* token) const {
    return SourceLocation::fromSource(compilationUnit(), token->getLine(),
                                      token->getCharPositionInLine());
  }
  /// Returns the SourceRange of the given ParseTree
  SourceRange sourceRangeOf(antlr4::tree::TerminalNode* token) const;
  /// Returns the SourceRange of the given ParseTree w
  SourceRange sourceRangeOf(antlr4::tree::TerminalNode* begin,
                            antlr4::tree::TerminalNode* end) const;

  /// Returns the given TerminalNode as identifier
  Identifier identifierOf(antlr4::tree::TerminalNode* node) const {
    return {poolString(node->getText()), sourceRangeOf(node)};
  }

protected:
  /// Returns an nullptr any as default value for performance improvement
  antlrcpp::Any defaultResult() override { return nullptr; }
  /// Never aggregate results as performance improvement
  antlrcpp::Any aggregateResult(antlrcpp::Any, antlrcpp::Any const&) override {
    return defaultResult();
  }
};

#endif // #ifndef BASIC_TREE_VISITOR_HPP_INCLUDED__
