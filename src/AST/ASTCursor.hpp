
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
#ifndef AST_CURSOR_HPP_INCLUDED__
#define AST_CURSOR_HPP_INCLUDED__

#include <utility>

class ASTNode;
enum class ASTKind;

/// Describes the depth on how far we descended in the AST level
enum class DepthLevel {
  /// We are on the top level scope, which is part of a UnitASTNode.
  /// Node we are faced with should inherit the TopLevelASTNode.
  TopLevel,
  /// We are inside a function declaration which means all nodes are
  /// transitive children of FunctionDeclASTNode.
  InsideFunctionDecl
};

/// Describes the depth on how far we descended in the meta level
enum class MetaDepthLevel {
  /// We are in no MetaDeclASTNode which is equivalent
  /// to the the default state.
  Outside,
  /// We are inside a MetaDeclASTNode and contributing the nodes
  /// to the instantiation.
  InsideMetaDecl,
  /// We are inside a meta computation and doing compile-time computations.
  /// Children aren't contributed to the instantiation anymore.
  InsideComputation
};

/// Represents a 1 dimensional depth in the AST
template <typename T> class ASTDepth {
  T depth_{};

public:
  ASTDepth() = default;
  explicit ASTDepth(T depth) : depth_(std::move(depth)) {}

  /// Returns true when we are in the given depth
  bool is(T depth) const { return depth_ == depth; }
  /// Returns the current depth
  T const& get() const { return depth_; }
  /// Sets the current depth
  void set(T depth) { depth_ = std::move(depth); }

  /// Descends into the AST which means we are increasing the depth level
  void descend(ASTKind kind);
  /// Ascend from the AST which means we are decreasing the depth level
  void ascend(ASTKind kind);
};

/// Represents a 2 dimensional cursor which tracks the AST and meta depth
class ASTCursor : protected ASTDepth<DepthLevel>,
                  protected ASTDepth<MetaDepthLevel> {
public:
  explicit ASTCursor(DepthLevel depth = DepthLevel{},
                     MetaDepthLevel metaDepth = MetaDepthLevel{})
      : ASTDepth<DepthLevel>(depth), ASTDepth<MetaDepthLevel>(metaDepth) {}

  /*bool is(DepthLevel level) const { return depth_ == level; }
  bool is(MetaDepthLevel level) const { return metaDepth_ == level; }*/
  bool is(DepthLevel depthLevel, MetaDepthLevel metaDepthLevel) const {
    return ASTDepth<DepthLevel>::is(depthLevel) &&
           ASTDepth<MetaDepthLevel>::is(metaDepthLevel);
  }

  /// Returns the current depth
  DepthLevel getDepth() const { return ASTDepth<DepthLevel>::get(); }
  /// Returns the current meta depth
  MetaDepthLevel getMetaDepth() const {
    return ASTDepth<MetaDepthLevel>::get();
  }

  /// Descends into the AST which means we are increasing the depth level
  void descend(ASTKind kind);
  /// Ascend from the AST which means we are decreasing the depth level
  void ascend(ASTKind kind);

  /// Returns true when the cursor points into a function decl
  bool isInsideFunctionDecl() const {
    return ASTDepth<DepthLevel>::get() >= DepthLevel::InsideFunctionDecl;
  }
  /// Returns true when the cursor points into a meta decl
  bool isInsideMetaDecl() const {
    return ASTDepth<MetaDepthLevel>::get() >= MetaDepthLevel::InsideMetaDecl;
  }
};

#endif // #ifndef AST_CURSOR_HPP_INCLUDED__
