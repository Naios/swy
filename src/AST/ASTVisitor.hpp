
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

#ifndef AST_VISITOR_HPP_INCLUDED__
#define AST_VISITOR_HPP_INCLUDED__

#include "ASTPredicate.hpp"
#include "ASTTraversal.hpp"

class ASTNode;
// Forward declare all ASTNodes
#define FOR_EACH_AST_NODE(NAME) class NAME##ASTNode;
#include "AST.inl"

/// Returns a default value for the given type
template <typename Type> struct DefaultValueFactory {
  Type operator()() const { return Type{}; }
};
template <> struct DefaultValueFactory<void> {};

/// A default merge implementation for the given type
template <typename Type> struct DefaultCombiner;
template <> struct DefaultCombiner<void> {};

/// An adaptable base class for visiting ASTNodes
template <typename ResultType = void,
          typename CombinerType = DefaultCombiner<ResultType>,
          typename DefaultValueFactoryType = DefaultValueFactory<ResultType>>
class ASTVisitor {
  CombinerType combiner_;
  DefaultValueFactoryType defaultFactory_;

public:
  ASTVisitor() = default;
  virtual ~ASTVisitor() = default;

#define FOR_EACH_AST_NODE(NAME)                                                \
  virtual ResultType visit(NAME##ASTNode const* node) {                        \
    return visitChildren(node);                                                \
  }
#include "AST.inl"

  ResultType visitChildren(ASTNode const* node) {
    /// Manual promote
    return traverseNode(node, [=](auto* promoted) {
      return visitChildrenImpl(std::common_type<ResultType>{}, promoted);
    });
  }

  virtual ResultType accept(ASTNode const* node) {
    return traverseNode(node, [=](auto* promoted) { return visit(promoted); });
  }

private:
  void visitChildrenImpl(std::common_type<void>, ASTNode const* node) {
    traverseNodeIf(node, pred::hasChildren(), [&](auto promoted) {
      for (auto child : promoted->children()) {
        accept(child);
      }
    });
  }
  template <typename R>
  ResultType visitChildrenImpl(std::common_type<R>, ASTNode const* node) {
    auto combined = defaultFactory_();
    traverseNodeIf(node, pred::hasChildren(), [&](auto promoted) {
      for (auto child : promoted->children()) {
        combined = combiner_(combined, accept(child));
      }
    });
    return combined;
  }
};

#endif // #ifndef AST_VISITOR_HPP_INCLUDED__
