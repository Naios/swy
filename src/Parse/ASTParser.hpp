
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

#ifndef AST_GENERATOR_HPP_INCLUDED__
#define AST_GENERATOR_HPP_INCLUDED__

#include <memory>
#include <tuple>
#include <utility>

#include "llvm/ADT/Optional.h"

#include "ASTContext.hpp"

class CompilationUnit;
class CompilationUnitASTNode;

namespace antlr4 {
class TokenStream;
}

/// Represents the result of an token stream parse action
class ASTParseResult {
  ASTContextRef astContext_;
  CompilationUnitASTNode* compilationUnit_;

public:
  ASTParseResult(ASTContextRef astContext,
                 CompilationUnitASTNode* compilationUnit)
      : astContext_(std::move(astContext)), compilationUnit_(compilationUnit) {}

  ASTContextRef const& getASTContext() const { return astContext_; }

  CompilationUnitASTNode* getCompilationUnit() const {
    return compilationUnit_;
  }
};

/// Is responsible for mapping the antlr parse ast to our independent ast
class ASTParser {
  using SharedTokenStream = std::shared_ptr<antlr4::TokenStream>;

  CompilationUnit* compilationUnit_;

public:
  explicit ASTParser(CompilationUnit* compilationUnit)
      : compilationUnit_(compilationUnit) {}

  /// Parses the given token stream
  llvm::Optional<ASTParseResult> parse(SharedTokenStream const& tokens) const;

private:
  /// Returns true when the the parsing can continue
  bool canContinue() const;
};

#endif // #ifndef AST_GENERATOR_HPP_INCLUDED__
