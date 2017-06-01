
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

#include "ASTLayout.hpp"

#include <cassert>

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/SaveAndRestore.h"

#include "ASTTraversal.hpp"

void ASTLayoutWriter::write(NonNull<ASTNode*> node) {
  directWrite(node);
  if (isNodeRequiringReduceMarker(*node)) {
    markReduce();
  }
}

ScopeLeaveAction ASTLayoutWriter::scopedWrite(NonNull<ASTNode*> node) {
  directWrite(node);
  return ScopeLeaveAction([this, node] {
    if (isNodeRequiringReduceMarker(*node)) {
      markReduce();
    }
  });
}

bool ASTLayoutWriter::isNodeRequiringReduceMarker(ASTNode const* node) {
  return traverseNode(
      node, decorate(identityOf<bool>(), pred::isRequiringReduceMarker()));
}

void ASTLayoutWriter::directWrite(NonNull<ASTNode*> node) {
  layout_.push_back(*node);
}

void ASTLayoutWriter::markReduce() {
  layout_.push_back(nullptr);
  ;
}

Nullable<ASTNode*> ASTLayoutReader::peek() const {
  assert((pos_ < layout_.size()) && "Didn't expected to be at the end!");
  return layout_[pos_];
}

Nullable<ASTNode*> ASTLayoutReader::shift() {
  auto current = peek();
  ++pos_;
  return current;
}

ASTLayoutReader::ScopedShift<ASTNode> ASTLayoutReader::scopedShift() {
  auto node = *shift();
  return ScopedShift<ASTNode>(
      node, ScopeLeaveAction([=] {
        if (ASTLayoutWriter::isNodeRequiringReduceMarker(node)) {
          reduce();
        }
      }));
}

void ASTLayoutReader::reduce() {
  assert(shouldReduce() && "Expected a reduce mark at the current position!");
  shift();
}

static void crawlExpand(ASTLayoutReader& reader) {
  while (!reader.shouldReduce()) {
    auto current = *reader.shift();
    if (ASTLayoutWriter::isNodeRequiringReduceMarker(current)) {
      crawlExpand(reader);
    }
  }
  reader.reduce();
}

void ASTLayoutReader::crawlCurrentScope(
    llvm::function_ref<void(ASTNode* node)> consumer) {
  // Restore the current cursor when leaving this function
  llvm::SaveAndRestore<decltype(pos_)> restore(pos_);

  while (!shouldReduce()) {
    auto current = *shift();
    consumer(current);
    if (ASTLayoutWriter::isNodeRequiringReduceMarker(current)) {
      crawlExpand(*this);
    }
  }
}

/// Helper method for calling the consume method of a given node
struct ConsumeDispatcher {
#define FOR_EACH_AST_NODE(NAME)                                                \
  static ASTNode* consumeHelper(ASTLayoutReader* reader,                       \
                                NAME##ASTNode const*) {                        \
    return reader->consume##NAME();                                            \
  }
#include "AST.inl"
};

ASTNode* ASTLayoutReader::consume() {
  return traverseNode(*peek(), [=](auto promoted) {
    return ConsumeDispatcher::consumeHelper(this, promoted);
  });
}

CompilationUnitASTNode* ASTLayoutReader::consumeCompilationUnit() {
  auto node = scopedShiftAs<CompilationUnitASTNode>();

  auto scope = enterConsistentScope();
  node->setScope(*scope);

  introduceScope(*node);

  while (!shouldReduce()) {
    node->addChild(consumeTopLevelDecl());
  }

  return *node;
}

MetaUnitASTNode* ASTLayoutReader::consumeMetaUnit() {
  auto node = scopedShiftAs<MetaUnitASTNode>();

  auto decl = llvm::cast<MetaDeclASTNode>(
      node->getInstantiation()->getDecl()->getDecl()->getDeclaringNode());

  node->setContainingUnit(decl->getContainingUnit());

  // Get the scope from the unit node the MetaDecl is declared in as parent
  auto parentScope = traverseNodeExpecting(
      node->getContainingUnit(), pred::isUnitNode(),
      [](UnitASTNode const* promoted) { return promoted->getScope(); });

  auto scope = enterConsistentScope(parentScope);
  node->setScope(*scope);

  introduceScope(*node);

  while (!shouldReduce()) {
    auto child = consumeTopLevelDecl();
    node->addChild(child);
  }

  return *node;
}

FunctionDeclASTNode* ASTLayoutReader::consumeFunctionDecl() {
  auto node = scopedShiftAs<FunctionDeclASTNode>();

  auto scope = enterTemporaryScope();
  node->setArgDeclList(consumeArgumentDeclList());

  if (is<AnonymousArgumentDeclASTNode>()) {
    node->setReturnType(consumeAnonymousArgumentDecl());
  }

  node->setBody(consumeStmt());
  return *node;
}

ArgumentDeclListASTNode* ASTLayoutReader::consumeArgumentDeclList() {
  auto node = scopedShiftAs<ArgumentDeclListASTNode>();
  while (!shouldReduce()) {
    node->addArgument(consumeAnonymousArgumentDecl());
  }
  return *node;
}

AnonymousArgumentDeclASTNode* ASTLayoutReader::consumeAnonymousArgumentDecl() {
  if (is<NamedArgumentDeclASTNode>()) {
    return consumeNamedArgumentDecl();
  }
  return shiftAs<AnonymousArgumentDeclASTNode>();
}

NamedArgumentDeclASTNode* ASTLayoutReader::consumeNamedArgumentDecl() {
  auto node = shiftAs<NamedArgumentDeclASTNode>();
  introduce(node);
  return node;
}

MetaDeclASTNode* ASTLayoutReader::consumeMetaDecl() {
  auto node = shiftAs<MetaDeclASTNode>();

  auto scope = enterTemporaryScope();
  node->setArgDeclList(consumeArgumentDeclList());

  // Enter the meta decl after the arguments were consumed
  auto mode = enterMetaDeclMode();
  node->setMetaContribution(consumeMetaContribution());
  return node;
}

GlobalConstantDeclASTNode* ASTLayoutReader::consumeGlobalConstantDecl() {
  auto node = shiftAs<GlobalConstantDeclASTNode>();
  node->setExpression(consumeConstantExpr());
  return node;
}

MetaContributionASTNode* ASTLayoutReader::consumeMetaContribution() {
  auto node = scopedShiftAs<MetaContributionASTNode>();
  while (!shouldReduce()) {
    node->addChild(consume());
  }
  return *node;
}

MetaInstantiationExprASTNode* ASTLayoutReader::consumeMetaInstantiationExpr() {
  auto node = scopedShiftAs<MetaInstantiationExprASTNode>();
  node->setDecl(consumeDeclRefExpr());
  while (!shouldReduce()) {
    node->addArgument(consumeExpr());
  }
  return *node;
}

StmtASTNode* ASTLayoutReader::consumeStmt() {
  // Call the specialized consume method
  return traverseNodeExpecting(
      peekAs<StmtASTNode>(), pred::isStmtNode(),
      [=](auto promoted) -> StmtASTNode* {
        return static_cast<StmtASTNode*>(
            ConsumeDispatcher::consumeHelper(this, promoted));
      });
}

ExprASTNode* ASTLayoutReader::consumeExpr() {
  // Call the specialized consume method
  return traverseNodeExpecting(
      peekAs<ExprASTNode>(), pred::isExprNode(),
      [=](auto promoted) -> ExprASTNode* {
        return static_cast<ExprASTNode*>(
            ConsumeDispatcher::consumeHelper(this, promoted));
      });
}

UnscopedCompoundStmtASTNode* ASTLayoutReader::consumeUnscopedCompoundStmt() {
  auto node = scopedShiftAs<UnscopedCompoundStmtASTNode>();

  while (!shouldReduce()) {
    node->addStatement(consumeStmt());
  }
  return *node;
}

CompoundStmtASTNode* ASTLayoutReader::consumeCompoundStmt() {
  auto node = scopedShiftAs<CompoundStmtASTNode>();
  auto scope = enterTemporaryScope();

  while (!shouldReduce()) {
    node->addStatement(consumeStmt());
  }
  return *node;
}

ReturnStmtASTNode* ASTLayoutReader::consumeReturnStmt() {
  auto node = scopedShiftAs<ReturnStmtASTNode>();
  if (!shouldReduce()) {
    node->setExpression(consumeExpr());
  }
  return *node;
}

ExpressionStmtASTNode* ASTLayoutReader::consumeExpressionStmt() {
  auto node = shiftAs<ExpressionStmtASTNode>();
  node->setExpression(consumeExpr());
  return node;
}

DeclStmtASTNode* ASTLayoutReader::consumeDeclStmt() {
  auto node = shiftAs<DeclStmtASTNode>();
  node->setExpression(consumeExpr());
  introduce(node);
  return node;
}

IfStmtASTNode* ASTLayoutReader::consumeIfStmt() {
  auto node = scopedShiftAs<IfStmtASTNode>();
  node->setExpression(consumeExpr());
  node->setTrueBranch(consumeStmt());
  if (!shouldReduce()) {
    node->setFalseBranch(consumeStmt());
  }
  return *node;
}

MetaIfStmtASTNode* ASTLayoutReader::consumeMetaIfStmt() {
  auto node = scopedShiftAs<MetaIfStmtASTNode>();

  {
    auto mode = enterMetaComputationMode();
    node->setExpression(consumeExpr());
  }

  node->setTrueBranch(consumeMetaContribution());

  if (!shouldReduce()) {
    node->setFalseBranch(consumeMetaContribution());
  }
  return *node;
}

MetaCalculationStmtASTNode* ASTLayoutReader::consumeMetaCalculationStmt() {
  auto node = shiftAs<MetaCalculationStmtASTNode>();

  auto listener = [&](NamedDeclContext* decl) { node->addExportedDecl(decl); };

  auto scope = enterInplaceScope(listener);

  auto mode = enterMetaComputationMode();
  node->setStmt(consumeStmt());

  return node;
}

DeclRefExprASTNode* ASTLayoutReader::consumeDeclRefExpr() {
  auto node = shiftAs<DeclRefExprASTNode>();

  if (isAllowedToResolveDecls()) {
    // Resolve declarations when we are allowed to
    if (auto decl = lookup(node->getName())) {
      node->setDecl(*decl);
    }
  }
  return node;
}

IntegerLiteralExprASTNode* ASTLayoutReader::consumeIntegerLiteralExpr() {
  return shiftAs<IntegerLiteralExprASTNode>();
}

BooleanLiteralExprASTNode* ASTLayoutReader::consumeBooleanLiteralExpr() {
  return shiftAs<BooleanLiteralExprASTNode>();
}

ErroneousExprASTNode* ASTLayoutReader::consumeErroneousExpr() {
  llvm_unreachable("You are joking :-)");
}

BinaryOperatorExprASTNode* ASTLayoutReader::consumeBinaryOperatorExpr() {
  auto node = shiftAs<BinaryOperatorExprASTNode>();
  node->setLeftExpr(consumeExpr());
  node->setRightExpr(consumeExpr());
  return node;
}

CallOperatorExprASTNode* ASTLayoutReader::consumeCallOperatorExpr() {
  auto node = scopedShiftAs<CallOperatorExprASTNode>();
  node->setCallee(consumeExpr());
  while (!shouldReduce()) {
    node->addExpression(consumeExpr());
  }
  return *node;
}

void ASTLayoutReader::introduceScope(ASTNode* parent) {
  auto metaUnit = llvm::dyn_cast<MetaUnitASTNode>(parent);

  crawlCurrentScope([&](ASTNode* child) {
    traverseNode(child, [&](auto* promoted) {
      // Set the parent unit of the node since those could
      // be needed when introducing the decl to resolve shadowing.
      staticIf(promoted, pred::isTopLevelNode(),
               [&](TopLevelASTNode* toplevel) {
                 toplevel->setContainingUnit(parent);
               });
      // Introduce top level decls
      staticIf(promoted, pred::isNamedDeclContext(), [&](auto* decl) {
        if (metaUnit) {
          // Set the exported node to the one which has the same name
          // as the meta decl.
          auto name = metaUnit->getInstantiation()->getDecl()->getName();

          if (decl->getName() == name) {
            metaUnit->setExportedNode(decl);
          }
        }
        introduce(decl);
      });
    });
  });
}

ASTNode* ASTLayoutReader::consumeTopLevelDecl() {
  return traverseNodeExpecting(
      *peek(), pred::isTopLevelNode(), [&](auto* promoted) {
        return ConsumeDispatcher::consumeHelper(this, promoted);
      });
}

ConstantExprASTNode* ASTLayoutReader::consumeConstantExpr() {
  return traverseNodeExpecting(
      *peek(), pred::isBaseOf<ConstantExprASTNode>(), [&](auto* promoted) {
        return static_cast<ConstantExprASTNode*>(
            ConsumeDispatcher::consumeHelper(this, promoted));
      });
}
