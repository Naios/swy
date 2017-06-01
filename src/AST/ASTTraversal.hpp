
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

#ifndef AST_TRAVERSAL_HPP_INCLUDED__
#define AST_TRAVERSAL_HPP_INCLUDED__

#include <type_traits>

#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "AST.hpp"
#include "Traits.hpp"

namespace traversal_detail {
template <typename Predicate, typename Callback>
auto returnTypeOf(Predicate predicate, Callback callback) {
  auto match = firstMatchOf(std::move(predicate),
#define FOR_EACH_AST_NODE(NAME) identityOf<NAME##ASTNode*>(),
#include "AST.inl"
                            identityOf<ASTNode>());

  (void)match;
  using MatchedType = typename decltype(match)::type;

  /*auto matchedAny = pred::isSameAs<ASTNode>()(match);
  (void)matchedAny;
  static_assert(!decltype(matchedAny)::value, "blub!");*/

  (void)callback;
  using ResultType = decltype(callback(std::declval<MatchedType>()));
  return identityOf<ResultType>(); // ;
}

template <typename T, typename ReturnType, typename Callback>
ReturnType traverseExpectedNodeImpl(std::true_type, T* node,
                                    identity<ReturnType>, Callback&& callback) {
  return std::forward<Callback>(callback)(node);
}

template <typename T, typename ReturnType, typename Callback>
ReturnType traverseExpectedNodeImpl(std::false_type, T* /*node*/,
                                    identity<ReturnType>,
                                    Callback&& /*callback*/) {
  llvm_unreachable("Expected the node to match the predicate!");
}
} // end namespace traversal_detail

/// Traverses the given ASTNode and calls the given callback with
/// the real class type of the ASTNode.
template <typename Callback>
auto traverseNode(ASTNode* node, Callback&& callback) {
  switch (node->getKind()) {
#define FOR_EACH_AST_NODE(NAME)                                                \
  case ASTKind::Kind##NAME:                                                    \
    return std::forward<Callback>(callback)(llvm::cast<NAME##ASTNode>(node));
#include "AST.inl"
    default:
      llvm_unreachable("The ASTNode has an unregistered type!");
  }
}

/// Traverses the given ASTNode and calls the given callback with
/// the real class type of the ASTNode.
template <typename Callback>
auto traverseNode(ASTNode const* node, Callback&& callback) {
  switch (node->getKind()) {
#define FOR_EACH_AST_NODE(NAME)                                                \
  case ASTKind::Kind##NAME:                                                    \
    return std::forward<Callback>(callback)(llvm::cast<NAME##ASTNode>(node));
#include "AST.inl"
    default:
      llvm_unreachable("The ASTNode has an unregistered type!");
  }
}

/// Traverses the node and expect it match the given predicate.
/// When the node doesn't match the predicate this will assert.
template <typename Predicate, typename Callback>
auto traverseNodeExpecting(ASTNode* node, Predicate&& predicate,
                           Callback&& callback) {

  auto returnType = traversal_detail::returnTypeOf(predicate, callback);

  return traverseNode(node, [&](auto promoted) {
    return traversal_detail::traverseExpectedNodeImpl(
        predicate(promoted), promoted, returnType,
        std::forward<Callback>(callback));
  });
}

/// Traverses the node and expect it match the given predicate.
/// When the node doesn't match the predicate this will assert.
template <typename Predicate, typename Callback>
auto traverseNodeExpecting(ASTNode const* node, Predicate&& predicate,
                           Callback&& callback) {

  auto returnType = traversal_detail::returnTypeOf(predicate, callback);

  return traverseNode(node, [&](auto promoted) {
    return traversal_detail::traverseExpectedNodeImpl(
        predicate(promoted), promoted, returnType,
        std::forward<Callback>(callback));
  });
}

/// Traverses the node only when the given static matcher returns a true type
/// for the node.
template <typename Predicate, typename Callback>
void traverseNodeIf(ASTNode* node, Predicate&& predicate, Callback&& callback) {
  traverseNode(node, invokeFilter(std::forward<Predicate>(predicate),
                                  std::forward<Callback>(callback)));
}

/// Traverses the node only when the given static matcher returns a true type
/// for the node.
template <typename Predicate, typename Callback>
void traverseNodeIf(ASTNode const* node, Predicate&& predicate,
                    Callback&& callback) {
  traverseNode(node, invokeFilter(std::forward<Predicate>(predicate),
                                  std::forward<Callback>(callback)));
}

#endif // #ifndef AST_TRAVERSAL_HPP_INCLUDED__
