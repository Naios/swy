
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

#ifndef AST_PREDICATE_HPP_INCLUDED__
#define AST_PREDICATE_HPP_INCLUDED__

#include <array>

#include "AST.hpp"
#include "Traits.hpp"

/// The namespace `pred` provides compile-time predicates for ASTNode's
namespace pred {
/// A static AST predicate which returns a true type:
/// - if the node provides a 'children()' method for iterating it's children.
inline auto hasChildren() {
  return validatorOf(
      [](auto* checked) -> decltype((void)checked->children()) {});
}

namespace detail {
template <typename>
struct known_size_of : std::integral_constant<std::size_t, 0U> {};

template <typename T, std::size_t Size>
struct known_size_of<std::array<T, Size>>
    : std::integral_constant<std::size_t, Size> {};

/// Returns a functor which tests it argument type with the given trait
template <template <class> class T> inline auto applyTrait() {
  return [](auto node) {
    (void)node;
    return T<plain_t<decltype(node)>>{};
  };
}

/// Returns a functor which tests it argument type with the given trait
/// and the curried argument.
template <template <class, class> class T, typename FirstArg>
inline auto applyTrait() {
  return [](auto node) {
    (void)node;
    return T<FirstArg, plain_t<decltype(node)>>{};
  };
}
} // end namespace detail

/// A static AST predicate which returns an integral constant:
/// - which indicates the known fixed size of a nodes children,
///   0 if the node has a dynamic size of children.
inline auto getKnownAmountOfChildren() {
  return [](auto* checked) {
    (void)checked;
    return detail::known_size_of<std::decay_t<decltype(checked->children())>>{};
  };
}

/// A static AST predicate which returns a true type:
/// - if the node provides a variable size of children
inline auto hasUnknownAmountOfChildren() {
  return [](auto* checked) {
    auto known = getKnownAmountOfChildren()(checked);
    (void)known;
    return std::integral_constant<bool, decltype(known)::value == 0>{};
  };
}

/// A static AST predicate which returns a true type:
/// - if the node requires a reduce marker
inline auto isRequiringReduceMarker() {
  return [](auto checked) {
    return conditionalEvaluate(hasChildren()(checked), checked,
                               hasUnknownAmountOfChildren(),
                               supplierOf<std::false_type>());
  };
}

/// A static AST predicate which returns a true type:
/// - if the node is a base of the given type.
template <typename Base> inline auto isBaseOf(identity<Base> = {}) {
  return detail::applyTrait<std::is_base_of, Base>();
}

/// A static AST predicate which returns a true type:
/// - if the node is a NamedDeclContext.
inline auto isNamedDeclContext() { return isBaseOf<NamedDeclContext>(); }

/// A static AST predicate which returns a true type:
/// - if the node is a top level ASTNode
inline auto isTopLevelNode() { return isBaseOf<TopLevelASTNode>(); };

/// A static AST predicate which returns a true type:
/// - if the node is a unit node
inline auto isUnitNode() { return isBaseOf<UnitASTNode>(); };

/// A static AST predicate which returns a true type:
/// - if the node is a StmtASTNode
inline auto isStmtNode() { return isBaseOf<StmtASTNode>(); };

/// A static AST predicate which returns a true type:
/// - if the node is a ExprASTNode
inline auto isExprNode() { return isBaseOf<ExprASTNode>(); };

/// A static AST predicate which returns a true type:
/// - if the node is a top level decl context
inline auto isTopLevelDeclContextNode() {
  return [](auto node) {
    return isNamedDeclContext()(node) && isTopLevelNode()(node);
  };
}

/// A static AST predicate which returns a true type:
/// - if the node is the the same as the given ASTNode
template <typename T> auto isSameAs(identity<T> = {}) {
  return detail::applyTrait<std::is_same, T>();
}

/// A static AST predicate which returns a true type:
/// - if the argument node is any of the given ones.
template <typename... T> auto isAnyNodeOf(T... nodes) {
  return [=](auto matchable) {
    return isAnyOf(plainify(identityOf(matchable)), identityOf(nodes)...);
  };
}

/// A static AST predicate which returns a true type:
/// - if the node is not trivially destructible
inline auto isNotTrivialDestructible() {
  return [](auto const* node) {
    (void)node;
    return std::integral_constant<bool, !std::is_trivially_destructible<
                                            plain_t<decltype(node)>>::value>{};
  };
}
} // end namespace pred

#endif // #ifndef AST_PREDICATE_HPP_INCLUDED__
