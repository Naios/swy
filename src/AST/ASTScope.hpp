
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

#ifndef AST_SCOPE_HPP_INCLUDED__
#define AST_SCOPE_HPP_INCLUDED__

#include <memory>

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"

#include "ASTFragment.hpp"
#include "Nullable.hpp"

class ASTContext;
class NamedDeclContext;

/// Represents a scoped visibility for identifiers that reference
/// to an unique NamedDeclASTNode which is represented by that identifier.
///
/// This class is similar to llvm::ScopedHashTable, however it provides
/// support for multiple living scopes inheriting from the same scope.
class ASTScope : public ASTFragment {
public:
  ASTScope() = default;
  ASTScope(ASTScope&&) = delete;
  ASTScope& operator=(ASTScope&&) = delete;

  /// Returns the parent of this scope
  virtual llvm::Optional<ASTScope const*> parent() const = 0;

  /// Inserts the given string into this scope
  virtual void insert(llvm::StringRef str, NamedDeclContext* node) = 0;

  /// Returns the NamedDeclASTNode visible in the current scope that is
  /// matching the given StringRef.
  virtual Nullable<NamedDeclContext*>
  lookupIdentifier(llvm::StringRef str) const = 0;

  /// Returns true when this scope is permanent within this context
  virtual bool isConsistent() const = 0;

  /// Returns the most obvious alternative in the current
  /// scope to the given string
  virtual Nullable<NamedDeclContext*> similarTo(llvm::StringRef str) const = 0;

  //// Allocates a temporary scope
  static std::unique_ptr<ASTScope>
  CreateTemporary(llvm::Optional<ASTScope const*> parent = llvm::None);

  virtual Nullable<NamedDeclContext*>
  similarTo(llvm::StringRef str, unsigned distance,
            NamedDeclContext* current) const = 0;
};

/// Represents a consistent AST scope which is kept accessible
/// within the AST for accessing global scope members.
class ConsistentASTScope : public ASTScope {
public:
  ConsistentASTScope() = default;

  /// Allocates a permanent scope for the given AST context
  static ConsistentASTScope* CreateConsistent(
      ASTContext* astContext,
      llvm::Optional<ConsistentASTScope const*> parent = llvm::None);
};

/// Represents an inplace AST scope which doesn't introduce a new logical layer
/// but provides the capability on listening on node introduces into the
/// current layer.
class InplaceASTScope : public ASTScope {
public:
  using ListenerType = llvm::function_ref<void(NamedDeclContext*)>;

  InplaceASTScope() = default;

  /// Allocates an inplace scope for the given AST context
  static std::unique_ptr<InplaceASTScope> CreateInplace(ASTScope* parent,
                                                        ListenerType listener);
};

#endif // #ifndef AST_SCOPE_HPP_INCLUDED__
