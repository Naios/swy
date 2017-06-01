
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

#include "ASTCursor.hpp"

#include <cassert>

#include "AST.hpp"

/*
bool operator==(ASTCursor const& other) const {
return is(other.depth_, metaDepth_);
}
bool operator!=(ASTCursor const& other) const { return !(*this == other); }

bool operator==(DepthLevel level) const { return is(level); }
bool operator<(DepthLevel level) const { return depth_ < level; }
bool operator<=(DepthLevel level) const { return depth_ <= level; }
bool operator>(DepthLevel level) const { return depth_ > level; }
bool operator>=(DepthLevel level) const { return depth_ >= level; }

bool operator==(MetaDepthLevel level) const { return is(level); }
bool operator<(MetaDepthLevel level) const { return metaDepth_ < level; }
bool operator<=(MetaDepthLevel level) const { return metaDepth_ <= level; }
bool operator>(MetaDepthLevel level) const { return metaDepth_ > level; }
bool operator>=(MetaDepthLevel level) const { return metaDepth_ >= level; }
*/

template <typename T> void transitionDepth(ASTKind kind, T&& apply) {
  switch (kind) {
    case ASTKind::KindFunctionDecl:
      // FunctionDeclASTNode: TopLevel -> InsideFunctionDecl
      std::forward<T>(apply)(DepthLevel::TopLevel,
                             DepthLevel::InsideFunctionDecl);
      break;
    default:
      break;
  }
}

template <typename T> void transitionMetaDepth(ASTKind kind, T&& apply) {
  switch (kind) {
    case ASTKind::KindMetaDecl:
      // MetaDeclASTNode: Outside -> Inside
      std::forward<T>(apply)(MetaDepthLevel::Outside,
                             MetaDepthLevel::InsideMetaDecl);
      break;
    case ASTKind::KindMetaCalculationStmt:
      // MetaCalculationStmtASTNode: Inside -> InsideComputation
      std::forward<T>(apply)(MetaDepthLevel::InsideMetaDecl,
                             MetaDepthLevel::InsideComputation);
      break;
    default:
      break;
  }
}

template <typename T> auto makeTransition(ASTDepth<T>* depth) {
  return [=](auto from, auto to) {
    assert(depth->is(from) && "Unbalanced transitioning!");
    depth->set(to);
  };
}

/// Passes the arguments in reversed order into the given functor
template <typename T> auto reverse(T&& apply) {
  return [apply = std::forward<T>(apply)](auto left, auto right) {
    return apply(right, left);
  };
}

template <> void ASTDepth<DepthLevel>::descend(ASTKind kind) {
  transitionDepth(kind, makeTransition(this));
}

template <> void ASTDepth<DepthLevel>::ascend(ASTKind kind) {
  transitionDepth(kind, reverse(makeTransition(this)));
}

template <> void ASTDepth<MetaDepthLevel>::descend(ASTKind kind) {
  transitionMetaDepth(kind, makeTransition(this));
}

template <> void ASTDepth<MetaDepthLevel>::ascend(ASTKind kind) {
  transitionMetaDepth(kind, reverse(makeTransition(this)));
}

template class ASTDepth<DepthLevel>;
template class ASTDepth<MetaDepthLevel>;

void ASTCursor::descend(ASTKind kind) {
  ASTDepth<DepthLevel>::descend(kind);
  ASTDepth<MetaDepthLevel>::descend(kind);
}

void ASTCursor::ascend(ASTKind kind) {
  ASTDepth<DepthLevel>::ascend(kind);
  ASTDepth<MetaDepthLevel>::ascend(kind);
}
