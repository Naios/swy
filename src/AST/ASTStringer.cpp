
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

#include "ASTStringer.hpp"

#include "llvm/Support/ErrorHandling.h"

#include "Formatting.hpp"

struct TypeStringer {
#define FOR_EACH_AST_NODE(NAME)                                                \
  \
static llvm::StringRef                                                         \
  toTypeStringImpl(NAME##ASTNode const*) {                                     \
    return #NAME;                                                              \
  \
}
#include "AST.inl"
};

llvm::StringRef ASTStringer::toTypeString(ASTNode const* node) {
  return traverseNode(node, [](auto promoted) {
    return TypeStringer::toTypeStringImpl(promoted);
  });
}

llvm::Optional<std::string>
ASTStringer::toStringImpl(NamedDeclContext const* decl) {
  return decl->getName()->str();
}

llvm::Optional<std::string>
ASTStringer::toStringImpl(DeclRefExprASTNode const* node) {
  if (node->isResolved()) {
    return fmt::format("{} <resolved to '{}'>", node->getName().getType(),
                       toTypeString(node->getDecl()->getDeclaringNode()));
  } else {
    return fmt::format("{} <unresolved>", node->getName().getType());
  }
}

llvm::Optional<std::string>
ASTStringer::toStringImpl(IntegerLiteralExprASTNode const* node) {
  return std::to_string(*node->getLiteral());
}

llvm::Optional<std::string>
ASTStringer::toStringImpl(BooleanLiteralExprASTNode const* node) {
  return node->getLiteral() ? std::string("true") : std::string("false");
}

llvm::Optional<std::string>
ASTStringer::toStringImpl(BinaryOperatorExprASTNode const* node) {
  switch (*node->getBinaryOperator()) {
#define EXPR_BINARY_OPERATOR(NAME, REP, ...)                                   \
  case ExprBinaryOperator::Operator##NAME:                                     \
    return std::string(REP);
#include "AST.inl"
    default:
      llvm_unreachable("Unhandled binary expr operator!");
  }
}
