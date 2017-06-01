
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

#include "BasicASTBuilder.hpp"

#include "llvm/ADT/StringSwitch.h"

#include "AST.hpp"
#include "ASTStringer.hpp"
#include "CompilationUnit.hpp"

BasicASTBuilder::~BasicASTBuilder() {
  assert(!isInAnyScope() && "Ended with imbalanced scopes!");
}

ASTScope* BasicASTBuilder::currentScope() const {
  assert(isInAnyScope() && "Tried to access an empty scope!");
  return *currentScope_;
}

template <typename T>
BasicScopeReference<T> replaceScope(Nullable<ASTScope*>& current,
                                    std::unique_ptr<T> replacement) {

  auto scope = replacement.get();

  auto previous = std::exchange(current, scope);

  auto recover = [&current, replacement = move(replacement), previous ] {
    assert(*current == replacement.get() && "Wrong scope replacement order!");
    current = previous;
  };

  return {std::move(recover), scope};
}

ScopeReference BasicASTBuilder::enterTemporaryScope() {
  // Take the ownership here since we are managing it in leaveScope
  auto temporary = ASTScope::CreateTemporary(currentScope());
  assert(!temporary->isConsistent() &&
         "Wanted a temporary scope but got a consistent one!");

  return replaceScope(currentScope_, move(temporary));
}

ConsistentScopeReference BasicASTBuilder::enterConsistentScope(
    llvm::Optional<ConsistentASTScope const*> parent) {

  assert(!isInAnyScope() ||
         currentScope()->isConsistent() &&
             "Tried to initialize a consistent scope on top of a "
             "temporary one!");

  auto consistent = ConsistentASTScope::CreateConsistent(astContext(), parent);

  auto previous = std::exchange(currentScope_, consistent);

  auto recover = [this, previous, consistent] {
    assert(*currentScope_ == consistent && "Wrong scope replacement order!");
    currentScope_ = previous;
  };

  return {std::move(recover), consistent};
}

InplaceScopeReference
BasicASTBuilder::enterInplaceScope(InplaceASTScope::ListenerType listener) {
  return replaceScope(currentScope_,
                      InplaceASTScope::CreateInplace(currentScope(), listener));
}

bool BasicASTBuilder::isAllowedToIntroduceDecls() const {
  return !isInMetaDeclMode();
}

/// Test whether the node is exported from the instantiated meta decl and
/// thus shadowing it in the current scope.
static bool isShadowingDeclFromExportedNode(NamedDeclContext const* current,
                                            NamedDeclContext const* previous) {
  auto metaDecl = llvm::dyn_cast<MetaDeclASTNode>(previous->getDeclaringNode());
  if (!metaDecl) {
    return false;
  }

  // The previous node is a toplevel decl and has the same name like the
  return traverseNode(current->getDeclaringNode(),
                      [metaDecl](auto promoted) -> bool {
                        return conditionalEvaluate(
                            pred::isTopLevelNode()(promoted), promoted,
                            [metaDecl](auto promoted) -> bool {
                              // The containing unit of the decl needs to be a
                              // meta unit
                              auto metaUnit = llvm::dyn_cast<MetaUnitASTNode>(
                                  promoted->getContainingUnit());
                              if (!metaUnit) {
                                return false;
                              }

                              // The meta decl of the instantiated meta unit
                              // needs to be
                              // the same as we want to shadow
                              if (metaUnit->getInstantiation()
                                      ->getDecl()
                                      ->getDecl()
                                      ->getDeclaringNode() != metaDecl) {
                                return false;
                              }

                              // Finally only the node which is exported from
                              // the meta unit
                              // is allowed to shadow it's meta decl
                              auto exporting = metaUnit->getExportedNode();
                              return exporting && (*exporting == promoted);
                            },
                            supplierOf(false));
                      });
}

bool BasicASTBuilder::isAllowedToShadowDecl(
    NamedDeclContext const* current, NamedDeclContext const* previous) const {

  if (isShadowingDeclFromExportedNode(current, previous)) {
    return true;
  }

  return false;
}

bool BasicASTBuilder::isAllowedToResolveDecls() const {
  return !isInMetaDeclMode();
}

ScopeLeaveAction BasicASTBuilder::enterMetaDeclMode() {
  assert(metaDeclMode_.is(MetaDepthLevel::Outside));
  return metaDeclMode_.set(MetaDepthLevel::InsideMetaDecl);
}

ScopeLeaveAction BasicASTBuilder::enterMetaComputationMode() {
  assert(metaDeclMode_.is(MetaDepthLevel::InsideMetaDecl));
  return metaDeclMode_.set(MetaDepthLevel::InsideComputation);
}

Nullable<NamedDeclContext*>
BasicASTBuilder::lookup(NamedDeclContext const* namedDecl) const {
  return lookup(namedDecl->getName());
}

Nullable<NamedDeclContext*>
BasicASTBuilder::lookup(Identifier const& identifier) const {
  auto decl = directLookup(identifier);
  if (!decl) {
    // The name is not known on this scope
    diagnosticEngine()->diagnose(Diagnostic::ErrorDeclarationUnknown,
                                 identifier, identifier);

    // Try to suggest the most similar node
    if (auto similar = similarDeclarationOf(identifier)) {
      diagnosticEngine()->diagnose(
          Diagnostic::NoteDidYouMeanQuestion, similar->getName(),
          ASTStringer::toTypeString(similar->getDeclaringNode()),
          similar->getName());

      // TODO Enable this in the future if we aren't in 'thesis' mode anymore.
      // return similar; // Auto correct it with the most similar node
    }
  }
  return decl;
}

void BasicASTBuilder::introduce(NamedDeclContext* namedDecl) const {
  // Never introduce anything when we aren't allowed to
  if (!isAllowedToIntroduceDecls()) {
    return;
  }

  // Lookup in the scope directly not to produce error messages
  auto previous = directLookup(namedDecl);
  // When we aren't allowed to shadow the decl produce an error message
  if (previous && !isAllowedToShadowDecl(namedDecl, *previous)) {
    assert((*previous != namedDecl) &&
           "Tried to introduce a named decl twice!");

    // Emit an error if there is a visible decl with the same name already
    diagnosticEngine()->diagnose(
        Diagnostic::ErrorNameTypeDeclaredAlready, namedDecl->getName(),
        ASTStringer::toTypeString(namedDecl->getDeclaringNode()),
        namedDecl->getName());
    diagnosticEngine()->diagnose(
        Diagnostic::NotePreviousTypeDeclarationHint, previous->getName(),
        ASTStringer::toTypeString(previous->getDeclaringNode()));
  } else {
    currentScope()->insert(*namedDecl->getName(), namedDecl);
  }
}

Nullable<NamedDeclContext*>
BasicASTBuilder::similarDeclarationOf(llvm::StringRef str) const {
  return currentScope()->similarTo(str);
}

Nullable<NamedDeclContext*>
BasicASTBuilder::similarDeclarationOf(Identifier const& identifier) const {
  return similarDeclarationOf(*identifier);
}

Nullable<NamedDeclContext*>
BasicASTBuilder::directLookup(llvm::StringRef str) const {
  return currentScope()->lookupIdentifier(str);
}

Nullable<NamedDeclContext*>
BasicASTBuilder::directLookup(Identifier const& identifier) const {
  return directLookup(*identifier);
}

Nullable<NamedDeclContext*>
BasicASTBuilder::directLookup(NamedDeclContext const* namedDecl) const {
  return directLookup(namedDecl->getName());
}
