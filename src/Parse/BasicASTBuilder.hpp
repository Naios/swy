
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

#ifndef BASIC_AST_BUILDER_HPP_INCLUDED__
#define BASIC_AST_BUILDER_HPP_INCLUDED__

#include <cassert>
#include <stack>
#include <type_traits>

#include "llvm/ADT/Optional.h"

#include "ASTContext.hpp"
#include "ASTCursor.hpp"
#include "ASTScope.hpp"
#include "BasicTreeSupport.hpp"
#include "Ownership.hpp"
#include "ScopeLeaveAction.hpp"

class CompilationUnit;
class BasicASTBuilder;
class DiagnosticEngine;
class NamedDeclContext;

/// Holds a reference to a scope and leaves it on destruction
template <typename T> class BasicScopeReference : public ScopeLeaveAction {
  T* scope_;

public:
  template <typename F, typename = std::enable_if_t<
                            std::is_constructible<ScopeLeaveAction, F>::value>>
  BasicScopeReference(F&& action, T* scope)
      : ScopeLeaveAction(std::forward<F>(action)), scope_(scope) {}

  T* operator->() const { return scope_; }
  T* operator*() const { return scope_; }
};

/// A reference to a temporary scope
using ScopeReference = BasicScopeReference<ASTScope>;
/// A reference to a consistent scope
using ConsistentScopeReference = BasicScopeReference<ConsistentASTScope>;
/// A reference to an inplace scope
using InplaceScopeReference = BasicScopeReference<InplaceASTScope>;

/// The ScopedMode can hold a state which is just valid for the current scope
/// or any of it's sub scopes. You may set a state through the `set` method.
template <typename T, T DefaultValue> class ScopedMode {
  std::stack<T> modeStack_;

public:
  ScopedMode() {}

  /// Returns true when the scope is in the given mode
  bool is(T const& mode) const { return mode == currentMode(); }

  /// Sets the scope into the given mode and returns an object
  /// which automatically restores the mode when leaving the scope
  ScopeLeaveAction set(T mode) {
    modeStack_.push(mode);
    return ScopeLeaveAction([this] {
      assert(!modeStack_.empty());
      modeStack_.pop();
    });
  }

  /// Returns true when the default mode is used
  bool isDefaulting() const { return modeStack_.empty(); }

private:
  T currentMode() const {
    return isDefaulting() ? DefaultValue : modeStack_.top();
  }
};

/// Provides support methods for building an AST from various sources
class BasicASTBuilder : public BasicTreeSupport {
  Nullable<ASTScope*> currentScope_;

  ScopedMode<MetaDepthLevel, MetaDepthLevel::Outside> metaDeclMode_;

public:
  BasicASTBuilder(CompilationUnit* compilationUnit, ASTContext* astContext)
      : BasicTreeSupport(compilationUnit, astContext) {}

  virtual ~BasicASTBuilder();

  /// Returns the current scope
  ASTScope* currentScope() const;
  /// Creates and enters a new temporary scope
  ScopeReference enterTemporaryScope();
  /// Enters a permanent scope
  ConsistentScopeReference enterConsistentScope(
      llvm::Optional<ConsistentASTScope const*> parent = llvm::None);
  /// Creates and enters a new inplace scope
  InplaceScopeReference
  enterInplaceScope(InplaceASTScope::ListenerType listener);

  /// Enters a meta declaration which changes the syntax only mode
  ScopeLeaveAction enterMetaDeclMode();
  /// Enters a meta computation which changes the syntax only mode
  ScopeLeaveAction enterMetaComputationMode();
  /// Returns true when we are in inside a meta declaration
  bool isInMetaDeclMode() const {
    return metaDeclMode_.is(MetaDepthLevel::InsideMetaDecl);
  }

  /// Looks the given identifier up in the current scope
  /// If the identifier can't be found it will yield an error message.
  Nullable<NamedDeclContext*> lookup(Identifier const& identifier) const;
  /// Looks the name of the given NamedDeclContext up in the current scope
  /// If the identifier can't be found it will yield an error message.
  Nullable<NamedDeclContext*> lookup(NamedDeclContext const* namedDecl) const;
  /// Introduces the given NamedDeclContext in the current scope
  /// If the identifier is already present it will yield an error message.
  void introduce(NamedDeclContext* namedDecl) const;
  /// Returns the most similar alternative to the given string
  Nullable<NamedDeclContext*> similarDeclarationOf(llvm::StringRef str) const;
  /// Returns the most similar alternative to the given identifier
  Nullable<NamedDeclContext*>
  similarDeclarationOf(Identifier const& identifier) const;

  /// Returns true when we are allowed to introduce declarations
  bool isAllowedToIntroduceDecls() const;
  /// Returns true when we are allowed to shadow a given decl
  bool isAllowedToShadowDecl(NamedDeclContext const* current,
                             NamedDeclContext const* previous) const;
  /// Returns true when we are allowed to resolve declarations
  bool isAllowedToResolveDecls() const;

private:
  /// Returns true when we are in at least one scope
  bool isInAnyScope() const { return !currentScope_.empty(); }

  /// Searches for the StringRef in the current scope without yielding an
  /// error message.
  Nullable<NamedDeclContext*> directLookup(llvm::StringRef str) const;
  /// Searches for the identifier in the current scope without yielding an
  /// error message.
  Nullable<NamedDeclContext*> directLookup(Identifier const& identifier) const;
  /// Searches for the NamedDeclContext in the current scope without yielding an
  /// error message.
  Nullable<NamedDeclContext*>
  directLookup(NamedDeclContext const* namedDecl) const;
};

#endif // #ifndef BASIC_AST_BUILDER_HPP_INCLUDED__
