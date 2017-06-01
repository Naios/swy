
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

#ifndef AST_STRINGER_HPP_INCLUDED__
#define AST_STRINGER_HPP_INCLUDED__

#include <string>
#include <type_traits>

#include "llvm/ADT/Optional.h"

#include "AST.hpp"
#include "ASTTraversal.hpp"

/// Converts the ASTNode to it's most important representing value
class ASTStringer {
  template <typename T, typename = std::enable_if_t<!std::is_base_of<
                            NamedDeclContext, std::decay_t<T>>::value>>
  static llvm::Optional<std::string> toStringImpl(T const* /*node*/) {
    return llvm::None;
  }

  // General implementation for decl contexts
  static llvm::Optional<std::string> toStringImpl(NamedDeclContext const* decl);

  static llvm::Optional<std::string>
  toStringImpl(DeclRefExprASTNode const* node);
  static llvm::Optional<std::string>
  toStringImpl(IntegerLiteralExprASTNode const* node);
  static llvm::Optional<std::string>
  toStringImpl(BooleanLiteralExprASTNode const* node);
  static llvm::Optional<std::string>
  toStringImpl(BinaryOperatorExprASTNode const* node);

public:
  /// Returns the type name of the given ASTNode.
  static llvm::StringRef toTypeString(ASTNode const* node);

  /// Returns the most relevant information about the given ASTNode
  /// if there is any.
  static llvm::Optional<std::string> toString(ASTNode const* node) {
    return traverseNode(
        node, [=](auto const* promoted) { return toStringImpl(promoted); });
  }
};

#endif // #ifndef AST_STRINGER_HPP_INCLUDED__
