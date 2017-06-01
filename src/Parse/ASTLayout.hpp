
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

#ifndef AST_LAYOUTER_HPP_INCLUDED__
#define AST_LAYOUTER_HPP_INCLUDED__

#include <utility>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Casting.h"

#include "AST.hpp"
#include "ASTPredicate.hpp"
#include "BasicASTBuilder.hpp"
#include "Nullable.hpp"
#include "ScopeLeaveAction.hpp"
#include "Traits.hpp"

using ASTLayout = std::vector<Nullable<ASTNode*>>;

/// A helper class for writing ASTLayout's
class ASTLayoutWriter {
  ASTLayout layout_;

public:
  ASTLayoutWriter() = default;
  explicit ASTLayoutWriter(ASTLayout layout) : layout_(std::move(layout)) {}

  /// Writes the node to the layout and closes it's scope immediately
  void write(NonNull<ASTNode*> node);
  /// Writes the node to the layout and closes it's scope when the
  /// leave action is released.
  ScopeLeaveAction scopedWrite(NonNull<ASTNode*> node);

  /// Returns the completed layout
  ASTLayout const& buildLayout() const & { return layout_; }
  /// Returns the completed layout
  ASTLayout buildLayout() && { return std::move(layout_); }

  /// Returns true when the given node requires a reduce mark to be
  /// written at the end of the layout after all it's children were written.
  static bool isNodeRequiringReduceMarker(ASTNode const* node);

  /// Directly writes the node to the writer without doing postactions
  /// like autoreducing.
  /// Note: this could malform the layout on misusage.
  void directWrite(NonNull<ASTNode*> node);
  /// Directly marks the current node as reduced
  /// Note: this could malform the layout on misusage.
  void markReduce();
};

/// Provides methods for layouting ASTNodes taken from continuous space,
/// this is currently implemented through a LL(0) parser.
///
/// The layouter accepts a vector of ASTNode's which is structured as following:
/// - ASTNode (the node itself)
///   - child (which is then an ASTNode itself)
///   - child
///   - child (and more children..., this structure applies recursively)
/// - nullptr (reduce marker - only present if the node can hold any child,
///            this is required for resolving shift reduce conflicts
///            which we encounter in compound statements for instance.)
///
/// TODO This class theoretically can be implemented reentrant which makes
///      it more efficient for consuming nodes.
class ASTLayoutReader : public BasicASTBuilder {
  llvm::ArrayRef<Nullable<ASTNode*>> layout_;
  std::size_t pos_;

  /// Represents the result of a scoped shift which reads the
  /// reduce mark when the shift is released.
  template <typename T> class ScopedShift {
    template <typename> friend class ScopedShift;

    T* node_;
    ScopeLeaveAction scopeLeaveAction_;

  public:
    ScopedShift(T* node, ScopeLeaveAction scopeLeaveAction)
        : node_(node), scopeLeaveAction_(std::move(scopeLeaveAction)) {}
    template <typename O>
    ScopedShift(ScopedShift<O>&& other)
        : node_(llvm::cast<T>(other.node_)),
          scopeLeaveAction_(std::move(other.scopeLeaveAction_)) {}

    T* operator->() const { return node_; }
    T* operator*() const { return node_; }
    explicit operator T*() const { return node_; }
  };

public:
  ASTLayoutReader(CompilationUnit* compilationUnit, ASTContext* astContext,
                  llvm::ArrayRef<Nullable<ASTNode*>> layout)
      : BasicASTBuilder(compilationUnit,
                        astContext /*TODO Remove ast context from here*/),
        layout_(layout), pos_(0) {}

  /// Returns the current element in the stream
  Nullable<ASTNode*> peek() const;
  /// Returns the current element in the stream as the given type
  template <typename T> T* peekAs() const { return llvm::cast<T>(*peek()); }
  /// Returns true when the current element in the stream
  /// is of the given type.
  template <typename T> bool is() const { return llvm::isa<T>(*peek()); }
  /// Returns the current element in the stream
  /// and shifts the position to the right.
  Nullable<ASTNode*> shift();
  /// Returns the current element in the stream
  /// and shifts the position to the right.
  /// Also reads the reduce marker when the scope is left.
  ScopedShift<ASTNode> scopedShift();
  /// Returns the current element in the stream as the given type
  /// and shifts the position to the right.
  template <typename T> T* shiftAs() {
    auto mustReduce = pred::isRequiringReduceMarker()(static_cast<T*>(nullptr));
    (void)mustReduce;
    static_assert(!decltype(mustReduce)::value,
                  "You should call scoped shifts for nodes which "
                  "require a reduce marker!");
    return llvm::cast<T>(*shift());
  }
  /// Returns the current element in the stream as the given type
  /// and shifts the position to the right.
  /// Also reads the reduce marker when the scope is left.
  template <typename T> ScopedShift<T> scopedShiftAs() {
    auto mustReduce = pred::isRequiringReduceMarker()(static_cast<T*>(nullptr));
    (void)mustReduce;
    static_assert(decltype(mustReduce)::value,
                  "You shouldn't call scoped shifts for nodes which "
                  "don't require a reduce marker!");
    return scopedShift();
  }

  /// Returns true when the layouter should reduce the current production
  /// which is marked by a nullptr at the current position
  bool shouldReduce() const { return !peek(); }
  /// Reduces the current context
  void reduce();

  /// Crawls nodes in the current scope and calls the consumer
  /// with the nodes found.
  void crawlCurrentScope(llvm::function_ref<void(ASTNode* node)> consumer);

  /// Consumes any ASTNode
  ASTNode* consume();
  /// Consumes any statements
  StmtASTNode* consumeStmt();
  /// Consumes any expressions
  ExprASTNode* consumeExpr();

/// Provide consumer methods for all types of ASTNode's
#define FOR_EACH_AST_NODE(NAME) NAME##ASTNode* consume##NAME();
#include "AST.inl"

private:
  /// This introduces all named decl nodes into the current scope
  /// which is required for two phase lookup.
  void introduceScope(ASTNode* parent);

  /// Consumes a top level decl
  ASTNode* consumeTopLevelDecl();
  /// Consumes a constant expression
  ConstantExprASTNode* consumeConstantExpr();
};

#endif // #ifndef AST_LAYOUTER_HPP_INCLUDED__
