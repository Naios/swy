
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

#ifndef AST_HPP_INCLUDED__
#define AST_HPP_INCLUDED__

#include <array>
#include <cstdint>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"

#include "ASTFragment.hpp"
#include "Nullable.hpp"
#include "SourceAnnotated.hpp"
#include "SourceLocation.hpp"

class ASTNode;
class StmtASTNode;
class ExprASTNode;
#define FOR_EACH_AST_NODE(NAME) class NAME##ASTNode;
#include "AST.inl"
class ConsistentASTScope;
class ConstantExprASTNode;

/// Defines the kind of the ASTNode which is one of the basic types
enum class ASTKind {
#define FOR_EACH_AST_NODE(NAME) Kind##NAME,
#include "AST.inl"
};

/// A sequence to iterate of the children of the ASTNode
using ASTChildSequence = llvm::SmallVector<ASTNode*, 10>;
/// A const sequence to iterate of the children of the ASTNode
using ConstASTChildSequence = llvm::SmallVector<ASTNode const*, 10>;

/// Represents the base class of an every AST node
class ASTNode : public ASTFragment {
  ASTKind const kind_;

public:
  explicit ASTNode(ASTKind kind) : kind_(kind) {}

  /// Returns the kind of the ASTNode
  ASTKind getKind() const { return kind_; }
  /// Returns true when the ASTNode is of the given type
  bool isKind(ASTKind kind) const { return getKind() == kind; }
};

/// Provides access to a name declaring node
class NamedDeclContext {
  Identifier name_;

public:
  explicit NamedDeclContext(Identifier const& name) : name_(name) {}
  virtual ~NamedDeclContext() = default;

  /// Returns the identifier of the node that represents the name
  Identifier const& getName() const { return name_; }
  /// Returns the name associated to the name
  virtual ASTNode* getDeclaringNode() = 0;
  /// Returns the name associated to the name
  virtual ASTNode const* getDeclaringNode() const = 0;

  /// Returns true when the declaration is a function
  bool isFunctionDecl() const;
  /// Returns true when the declaration is a variable
  bool isVarDecl() const;
  /// Returns true when the declaration is a meta decl
  bool isMetaDecl() const;
  /// Returns true when the declaration is a global constant
  bool isGlobalConstant() const;
};

/// Generalized base class for all top level container ASTNode's such as:
/// - CompilationUnitASTNode
/// - MetaUnitASTNode
class UnitASTNode {
  NonNull<ConsistentASTScope*> scope_;
  std::vector<ASTNode*> children_;

public:
  UnitASTNode() = default;
  virtual ~UnitASTNode() = default;

  void setScope(ConsistentASTScope* scope) { scope_ = scope; }
  ConsistentASTScope const* getScope() const { return *scope_; }

  void addChild(ASTNode* child) { children_.push_back(child); }
  llvm::ArrayRef<ASTNode*> children() { return children_; }
  llvm::ArrayRef<ASTNode const*> children() const { return children_; }
};

/// Generalized base class for top level ASTNodes which are
/// contained in a UnitASTNode such as:
/// - MetaUnitASTNode
/// - FunctionDeclASTNode
/// - MetaDeclASTNode
class TopLevelASTNode {
  NonNull<ASTNode const*> containingUnit_;

public:
  TopLevelASTNode() = default;
  virtual ~TopLevelASTNode() = default;

  void setContainingUnit(ASTNode const* node) { containingUnit_ = node; }
  ASTNode const* getContainingUnit() const { return *containingUnit_; }
};

/// Represents a compilation unit of a source file
class CompilationUnitASTNode : public ASTNode, public UnitASTNode {
public:
  explicit CompilationUnitASTNode() : ASTNode(ASTKind::KindCompilationUnit) {}

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindCompilationUnit);
  }
};

/// Represents the result of an evaluation of a MetaDecl
class MetaUnitASTNode : public ASTNode,
                        public UnitASTNode,
                        public TopLevelASTNode,
                        public IntermediateNode {

  NonNull<MetaInstantiationExprASTNode const*> instantiation_;
  Nullable<ASTNode*> exportedNode_;

public:
  explicit MetaUnitASTNode(MetaInstantiationExprASTNode const* instantiation)
      : ASTNode(ASTKind::KindMetaUnit), instantiation_(instantiation) {}

  MetaInstantiationExprASTNode const* getInstantiation() const {
    return *instantiation_;
  }

  void setExportedNode(ASTNode* node) { exportedNode_ = node; }
  Nullable<ASTNode*> getExportedNode() { return exportedNode_; }
  Nullable<ASTNode const*> getExportedNode() const { return exportedNode_; }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindMetaUnit);
  }
};

/// Represents the declaration of a function
class FunctionDeclASTNode : public ASTNode,
                            public TopLevelASTNode,
                            public NamedDeclContext {

  NonNull<ArgumentDeclListASTNode*> arguments_;
  Nullable<AnonymousArgumentDeclASTNode*> returnType_;
  StmtASTNode* body_ = nullptr;

public:
  explicit FunctionDeclASTNode(Identifier const& name)
      : ASTNode(ASTKind::KindFunctionDecl), NamedDeclContext(name) {}

  void setArgDeclList(ArgumentDeclListASTNode* arguments) {
    arguments_ = arguments;
  }
  ArgumentDeclListASTNode* getArgDeclList() { return *arguments_; }
  ArgumentDeclListASTNode* getArgDeclList() const { return *arguments_; }

  void setReturnType(AnonymousArgumentDeclASTNode* returnType) {
    assert(!returnType_ && "Return Type already set!");
    returnType_ = returnType;
  }
  Nullable<AnonymousArgumentDeclASTNode*> getReturnType() {
    return returnType_;
  }
  Nullable<AnonymousArgumentDeclASTNode const*> getReturnType() const {
    return returnType_;
  }

  void setBody(StmtASTNode* body) { body_ = body; }
  StmtASTNode* getBody() { return body_; }
  StmtASTNode const* getBody() const { return body_; }

  ASTNode* getDeclaringNode() override { return this; }
  ASTNode const* getDeclaringNode() const override { return this; }

  llvm::SmallVector<ASTNode*, 3> children();
  llvm::SmallVector<ASTNode const*, 3> children() const;

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindFunctionDecl);
  }
};

/// Represents a list of AnonymousArgumentDeclASTNode
class ArgumentDeclListASTNode : public ASTNode {
  llvm::SmallVector<AnonymousArgumentDeclASTNode*, 2> arguments_;

public:
  explicit ArgumentDeclListASTNode() : ASTNode(ASTKind::KindArgumentDeclList) {}

  void addArgument(AnonymousArgumentDeclASTNode* argument) {
    arguments_.push_back(argument);
  }
  llvm::ArrayRef<AnonymousArgumentDeclASTNode*> children() {
    return arguments_;
  }
  llvm::ArrayRef<AnonymousArgumentDeclASTNode const*> children() const {
    return arguments_;
  }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindArgumentDeclList);
  }
};

/// Represents the definition of a function argument or
/// return type which is not named (anonymous).
class AnonymousArgumentDeclASTNode : public ASTNode {
  // Type isn't stored because we only allow int
  // NamedDeclASTNode* type_;

public:
  explicit AnonymousArgumentDeclASTNode(
      ASTKind kind = ASTKind::KindAnonymousArgumentDecl)
      : ASTNode(kind) {}

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindAnonymousArgumentDecl) ||
           node->isKind(ASTKind::KindNamedArgumentDecl);
  }
};

/// Represents the definition of a function argument or
/// return type which is named.
class NamedArgumentDeclASTNode : public AnonymousArgumentDeclASTNode,
                                 public NamedDeclContext {

public:
  explicit NamedArgumentDeclASTNode(Identifier const& name)
      : AnonymousArgumentDeclASTNode(ASTKind::KindNamedArgumentDecl),
        NamedDeclContext(name) {}

  ASTNode* getDeclaringNode() override { return this; }
  ASTNode const* getDeclaringNode() const override { return this; }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindNamedArgumentDecl);
  }
};

/// Represents the declaration of a meta scope
class MetaDeclASTNode : public ASTNode,
                        public TopLevelASTNode,
                        public NamedDeclContext,
                        public IntermediateNode {

  NonNull<ArgumentDeclListASTNode*> arguments_;
  Nullable<MetaContributionASTNode*> contribution_;

public:
  explicit MetaDeclASTNode(Identifier const& name)
      : ASTNode(ASTKind::KindMetaDecl), NamedDeclContext(name) {}

  void setArgDeclList(ArgumentDeclListASTNode* arguments) {
    arguments_ = arguments;
  }
  ArgumentDeclListASTNode* getArgDeclList() { return *arguments_; }
  ArgumentDeclListASTNode* getArgDeclList() const { return *arguments_; }

  /// Sets the meta contribution of this node
  void setMetaContribution(MetaContributionASTNode* contribution) {
    contribution_ = contribution;
  }
  /// Returns the meta contribution of this node
  MetaContributionASTNode* getContribution() { return *contribution_; }
  /// Returns the meta contribution of this node
  MetaContributionASTNode const* getContribution() const {
    return *contribution_;
  }

  ASTNode* getDeclaringNode() override { return this; }
  ASTNode const* getDeclaringNode() const override { return this; }

  std::array<ASTNode*, 2> children();
  std::array<ASTNode const*, 2> children() const;

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindMetaDecl);
  }
};

/// Represents a global constant declaration
class GlobalConstantDeclASTNode : public ASTNode,
                                  public TopLevelASTNode,
                                  public NamedDeclContext {

  NonNull<ConstantExprASTNode*> expression_;

public:
  explicit GlobalConstantDeclASTNode(Identifier const& name)
      : ASTNode(ASTKind::KindGlobalConstantDecl), NamedDeclContext(name) {}

  void setExpression(ConstantExprASTNode* expression) {
    expression_ = expression;
  }
  ConstantExprASTNode* getExpression() { return *expression_; }
  ConstantExprASTNode const* getExpression() const { return *expression_; }

  ASTNode* getDeclaringNode() override { return this; }
  ASTNode const* getDeclaringNode() const override { return this; }

  std::array<ConstantExprASTNode*, 1> children() { return {{*expression_}}; }
  std::array<ConstantExprASTNode const*, 1> children() const {
    return {{*expression_}};
  }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindGlobalConstantDecl);
  }
};

/// Represents a node which contributes it's child nodes when
/// instantiated in a meta declaration.
class MetaContributionASTNode : public ASTNode, public IntermediateNode {
  SourceRange range_;
  ASTChildSequence children_;

public:
  explicit MetaContributionASTNode(SourceRange range)
      : ASTNode(ASTKind::KindMetaContribution), range_(range) {}

  SourceRange getSourceRange() const { return range_; }

  /// Adds a ASTNode as child
  void addChild(ASTNode* node) { children_.push_back(node); }
  llvm::ArrayRef<ASTNode*> children() { return children_; }
  llvm::ArrayRef<ASTNode const*> children() const { return children_; }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindMetaContribution);
  }
};

/// Represents a single (one line) statement
class StmtASTNode : public ASTNode {

public:
  explicit StmtASTNode(ASTKind kind) : ASTNode(kind) {}

  static bool classof(ASTNode const* node);
};

class BasicCompoundStmtASTNode : public StmtASTNode {
  llvm::SmallVector<StmtASTNode*, 12> statements_;

public:
  explicit BasicCompoundStmtASTNode(ASTKind kind) : StmtASTNode(kind) {}

  void addStatement(StmtASTNode* statement) {
    statements_.push_back(statement);
  }

  llvm::ArrayRef<StmtASTNode*> children() { return statements_; }
  llvm::ArrayRef<StmtASTNode const*> children() const { return statements_; }
};

/// Represents multiple statements which lie in it's own scope
class CompoundStmtASTNode : public BasicCompoundStmtASTNode {
public:
  CompoundStmtASTNode() : BasicCompoundStmtASTNode(ASTKind::KindCompoundStmt) {}

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindCompoundStmt);
  }
};

/// Statements that are stored together which rely in the same scope as
/// it's neighbors.
class UnscopedCompoundStmtASTNode : public BasicCompoundStmtASTNode {
public:
  UnscopedCompoundStmtASTNode()
      : BasicCompoundStmtASTNode(ASTKind::KindUnscopedCompoundStmt) {}

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindUnscopedCompoundStmt);
  }
};

/// A return statement which returns a certain expression
class ReturnStmtASTNode : public StmtASTNode {
  Nullable<ExprASTNode*> expression_;

public:
  explicit ReturnStmtASTNode() : StmtASTNode(ASTKind::KindReturnStmt) {}

  void setExpression(ExprASTNode* expr) { expression_ = expr; }
  Nullable<ExprASTNode*> getExpression() { return expression_; }
  Nullable<ExprASTNode const*> getExpression() const { return expression_; }

  Nullable<ExprASTNode*> children() { return expression_; }
  Nullable<ExprASTNode const*> children() const { return expression_; }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindReturnStmt);
  }
};

template <typename T> class BasicIfStmtASTNode : public StmtASTNode {
  NonNull<ExprASTNode*> expression_;
  NonNull<T*> trueBranch_;
  Nullable<T*> falseBranch_;

public:
  explicit BasicIfStmtASTNode(ASTKind kind) : StmtASTNode(kind) {}

  void setExpression(ExprASTNode* expression) { expression_ = expression; }
  ExprASTNode* getExpression() { return *expression_; }
  ExprASTNode const* getExpression() const { return *expression_; }

  void setTrueBranch(T* trueBranch) { trueBranch_ = trueBranch; }
  T* getTrueBranch() { return *trueBranch_; }
  T const* getTrueBranch() const { return *trueBranch_; }

  void setFalseBranch(T* falseBranch) { falseBranch_ = falseBranch; }
  Nullable<T*> getFalseBranch() { return falseBranch_; }
  Nullable<T const*> getFalseBranch() const { return falseBranch_; }

  llvm::SmallVector<ASTNode*, 3> children() {
    llvm::SmallVector<ASTNode*, 3> seq{*expression_, *trueBranch_};
    if (falseBranch_) {
      seq.push_back(*falseBranch_);
    }
    return seq;
  }

  llvm::SmallVector<ASTNode const*, 3> children() const {
    llvm::SmallVector<ASTNode const*, 3> seq{*expression_, *trueBranch_};
    if (falseBranch_) {
      seq.push_back(*falseBranch_);
    }
    return seq;
  }
};

/// A conditional if statement
class IfStmtASTNode : public BasicIfStmtASTNode<StmtASTNode> {
public:
  explicit IfStmtASTNode() : BasicIfStmtASTNode(ASTKind::KindIfStmt) {}

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindIfStmt);
  }
};

/// A conditional meta if statement which makes it's AST children available
/// dependent on the given expression
class MetaIfStmtASTNode : public BasicIfStmtASTNode<MetaContributionASTNode>,
                          public IntermediateNode {
public:
  explicit MetaIfStmtASTNode() : BasicIfStmtASTNode(ASTKind::KindMetaIfStmt) {}

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindMetaIfStmt);
  }
};

/// A statement which evaluates an expression with dropping the result
class ExpressionStmtASTNode : public StmtASTNode {
  NonNull<ExprASTNode*> expression_;

public:
  explicit ExpressionStmtASTNode() : StmtASTNode(ASTKind::KindExpressionStmt) {}

  void setExpression(ExprASTNode* expression) { expression_ = expression; }
  ExprASTNode* getExpression() { return *expression_; }
  ExprASTNode const* getExpression() const { return *expression_; }

  std::array<ExprASTNode*, 1> children();
  std::array<ExprASTNode const*, 1> children() const;

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindExpressionStmt);
  }
};

/// Represents a variable declaration
class DeclStmtASTNode : public StmtASTNode, public NamedDeclContext {
  NonNull<ExprASTNode*> expression_;

public:
  explicit DeclStmtASTNode(Identifier const& name)
      : StmtASTNode(ASTKind::KindDeclStmt), NamedDeclContext(name) {}

  void setExpression(ExprASTNode* expression) { expression_ = expression; }
  ExprASTNode* getExpression() { return *expression_; }
  ExprASTNode const* getExpression() const { return *expression_; }

  ASTNode* getDeclaringNode() override { return this; }
  ASTNode const* getDeclaringNode() const override { return this; }

  std::array<ExprASTNode*, 1> children();
  std::array<ExprASTNode const*, 1> children() const;

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindDeclStmt);
  }
};

/// A statement which is evaluated at compile-time and that makes
/// it's variables available to scopes below.
class MetaCalculationStmtASTNode : public StmtASTNode, public IntermediateNode {
  NonNull<StmtASTNode*> stmt_;
  llvm::SmallVector<NamedDeclContext*, 5> exportedDecls_;

public:
  explicit MetaCalculationStmtASTNode()
      : StmtASTNode(ASTKind::KindMetaCalculationStmt) {}

  void setStmt(StmtASTNode* stmt) { stmt_ = stmt; }
  StmtASTNode* getStmt() { return *stmt_; }
  StmtASTNode const* getStmt() const { return *stmt_; }

  std::array<StmtASTNode*, 1> children();
  std::array<StmtASTNode const*, 1> children() const;

  void addExportedDecl(NamedDeclContext* decl) {
    exportedDecls_.push_back(decl);
  }
  llvm::ArrayRef<NamedDeclContext*> getExportedDecls() {
    return exportedDecls_;
  }
  llvm::ArrayRef<NamedDeclContext const*> getExportedDecls() const {
    return exportedDecls_;
  }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindMetaCalculationStmt);
  }
};

/// An expression which can be composed from other expressions
class ExprASTNode : public ASTNode {
public:
  explicit ExprASTNode(ASTKind kind) : ASTNode(kind) {}

  static bool classof(ASTNode const* node);
};

/// References a visible declaration
class DeclRefExprASTNode : public ExprASTNode {
  Identifier name_;
  Nullable<NamedDeclContext*> decl_;

public:
  explicit DeclRefExprASTNode(Identifier const& name)
      : ExprASTNode(ASTKind::KindDeclRefExpr), name_(name) {}

  Identifier const& getName() const { return name_; }

  void setDecl(NamedDeclContext* decl) { decl_ = decl; }
  Nullable<NamedDeclContext*> getDecl() const { return decl_; }

  /// Returns true when the decl ref is resolved
  bool isResolved() const { return bool(decl_); }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindDeclRefExpr);
  }
};

class MetaInstantiationExprASTNode : public ExprASTNode {
  NonNull<DeclRefExprASTNode*> decl_;
  llvm::SmallVector<ExprASTNode*, 3> arguments_;
  SourceRange range_;

public:
  explicit MetaInstantiationExprASTNode(SourceRange range)
      : ExprASTNode(ASTKind::KindMetaInstantiationExpr), range_(range) {}

  void setDecl(DeclRefExprASTNode* decl) { decl_ = decl; }
  DeclRefExprASTNode* getDecl() { return *decl_; }
  DeclRefExprASTNode const* getDecl() const { return *decl_; }

  void addArgument(ExprASTNode* node) { arguments_.push_back(node); }
  llvm::ArrayRef<ExprASTNode*> getArguments() { return arguments_; }
  llvm::ArrayRef<ExprASTNode const*> getArguments() const { return arguments_; }

  llvm::SmallVector<ExprASTNode*, 4> children();
  llvm::SmallVector<ExprASTNode const*, 4> children() const;

  SourceRange getSourceRange() const { return range_; }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindMetaInstantiationExpr);
  }
};

/// References a constant expression
class ConstantExprASTNode : public ExprASTNode {
public:
  explicit ConstantExprASTNode(ASTKind kind) : ExprASTNode(kind) {}
};

/// References an integer literal
class IntegerLiteralExprASTNode : public ConstantExprASTNode {
  RangeAnnotated<std::int32_t> literal_;

public:
  explicit IntegerLiteralExprASTNode(
      RangeAnnotated<std::int32_t> const& literal)
      : ConstantExprASTNode(ASTKind::KindIntegerLiteralExpr),
        literal_(literal) {}

  RangeAnnotated<std::int32_t> getLiteral() const { return literal_; }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindIntegerLiteralExpr);
  }
};

/// References an integer literal
class BooleanLiteralExprASTNode : public ConstantExprASTNode {
  RangeAnnotated<bool> literal_;

public:
  explicit BooleanLiteralExprASTNode(RangeAnnotated<bool> literal)
      : ConstantExprASTNode(ASTKind::KindBooleanLiteralExpr),
        literal_(literal) {}

  RangeAnnotated<bool> getLiteral() const { return literal_; }

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindBooleanLiteralExpr);
  }
};

/// Represents an error expression to keep the AST valid
class ErroneousExprASTNode : public ExprASTNode {
public:
  ErroneousExprASTNode() : ExprASTNode(ASTKind::KindErroneousExpr) {}

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindErroneousExpr);
  }
};

/// Represents the operator of a binary operation
enum class ExprBinaryOperator {
#define EXPR_BINARY_OPERATOR(NAME, REP, ...) Operator##NAME,
#include "AST.inl"
};

/// References a binary operator call
class BinaryOperatorExprASTNode : public ExprASTNode {
  RangeAnnotated<ExprBinaryOperator> binaryOperator_;
  NonNull<ExprASTNode*> left_;
  NonNull<ExprASTNode*> right_;

public:
  explicit BinaryOperatorExprASTNode(
      RangeAnnotated<ExprBinaryOperator> binaryOperator)
      : ExprASTNode(ASTKind::KindBinaryOperatorExpr),
        binaryOperator_(binaryOperator) {}

  RangeAnnotated<ExprBinaryOperator> const& getBinaryOperator() const {
    return binaryOperator_;
  }

  void setLeftExpr(ExprASTNode* left) { left_ = left; }
  ExprASTNode* getLeftExpr() { return *left_; }
  ExprASTNode const* getLeftExpr() const { return *left_; }

  void setRightExpr(ExprASTNode* right) { right_ = right; }
  ExprASTNode* getRightExpr() { return *right_; }
  ExprASTNode const* getRightExpr() const { return *right_; }

  std::array<ExprASTNode*, 2> children();
  std::array<ExprASTNode const*, 2> children() const;

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindBinaryOperatorExpr);
  }
};

/// References a call operator
class CallOperatorExprASTNode : public ExprASTNode {
  NonNull<ExprASTNode*> callee_;
  llvm::SmallVector<ExprASTNode*, 3> expressions_;

public:
  CallOperatorExprASTNode() : ExprASTNode(ASTKind::KindCallOperatorExpr) {}

  void setCallee(ExprASTNode* callee) { callee_ = callee; }
  ExprASTNode* getCallee() { return *callee_; }
  ExprASTNode const* getCallee() const { return *callee_; }

  void addExpression(ExprASTNode* expr) { expressions_.push_back(expr); }
  llvm::ArrayRef<ExprASTNode*> getExpressions() { return expressions_; }
  llvm::ArrayRef<ExprASTNode const*> getExpressions() const {
    return expressions_;
  }

  ASTChildSequence children();
  ConstASTChildSequence children() const;

  static bool classof(ASTNode const* node) {
    return node->isKind(ASTKind::KindCallOperatorExpr);
  }
};

// AST.hpp
// ASTNode.hpp
// ASTDeclNode.hpp
// ASTStmtNode.hpp
// ASTExprNode.hpp

#endif // #ifndef AST_HPP_INCLUDED__
