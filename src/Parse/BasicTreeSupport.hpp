
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

#ifndef BASIC_TREE_SUPPORT_HPP_INCLUDED__
#define BASIC_TREE_SUPPORT_HPP_INCLUDED__

#include <type_traits>

#include "ASTContext.hpp"
#include "SourceAnnotated.hpp"

class CompilationUnit;
class DiagnosticEngine;

/// Provides basic support methods for building an AST
class BasicTreeSupport {
private:
  CompilationUnit* compilationUnit_;
  ASTContext* astContext_;

public:
  BasicTreeSupport(CompilationUnit* compilationUnit, ASTContext* astContext)
      : compilationUnit_(compilationUnit), astContext_(astContext) {}

  /// Returns the CompilationUnit for the current processed AST
  CompilationUnit* const& compilationUnit() const { return compilationUnit_; }
  /// Returns the ASTContext responsible for the current processed AST
  ASTContext* astContext() const { return astContext_; }
  /// Allocates the given type T on the ASTContext
  template <typename T, typename... Args> auto allocate(Args&&... args) {
    return astContext_->allocate<T, Args...>(std::forward<Args>(args)...);
  }
  /// Returns the DiagnosticEngine which is used
  /// to emit messages about the source code
  DiagnosticEngine* diagnosticEngine() const;

  /// Pools the given string into the ASTContext
  llvm::StringRef poolString(llvm::StringRef str) const {
    return astContext_->poolString(str);
  }

  /// Annotates the given type with a source range
  template <typename T>
  RangeAnnotated<std::decay_t<T>> annotate(T type, SourceRange const& range) {
    return {std::move(type), range};
  }
};

#endif // #ifndef BASIC_TREE_VISITOR_HPP_INCLUDED__
