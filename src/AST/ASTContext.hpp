
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

#ifndef AST_CONTEXT_HPP_INCLUDED__
#define AST_CONTEXT_HPP_INCLUDED__

#include <deque>
#include <memory>
#include <type_traits>
#include <unordered_set>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Allocator.h"

#include "ASTFragment.hpp"
#include "ASTPredicate.hpp"
#include "Hash.hpp"
#include "Traits.hpp"

class ASTNode;

/// Represents the context of an abstract syntax tree
///
/// Contains the allocated memory for it and also keeps
/// fragments that aren't part of the AST accessible.
class ASTContext {
  llvm::BumpPtrAllocator allocator_;
  std::deque<ASTFragment*> allocated_;

  // We can ensure that all strings are needed due to the lifetime
  // of this context so we don't need a reference counted pool
  // like llvm::StringPool.
  std::unordered_set<llvm::StringRef, StringRefHasher> stringPool_;

public:
  ASTContext() = default;
  ~ASTContext();

  /// Allocates an ASTNode object of the given type inside the ASTContext.
  /// The object automatically gets destructed aside with the ASTContext,
  /// making it unnecessary to manage it's memory.
  template <typename T, typename... Args> T* allocate(Args&&... args) {
    static_assert(std::is_base_of<ASTFragment, T>::value,
                  "Can only allocate ASTFragment's!");
    T* allocated = static_cast<T*>(allocator_.Allocate(sizeof(T), alignof(T)));
    new (allocated) T(std::forward<Args>(args)...);
    staticIf(allocated, pred::isNotTrivialDestructible(),
             [&](ASTFragment* fragment) { allocated_.push_back(fragment); });
    return allocated;
  }

  /// Pools the string into the internal string table
  llvm::StringRef poolString(llvm::StringRef str);
};

/// Represents a shared instance of an ASTContext
using ASTContextRef = std::shared_ptr<ASTContext>;

#endif // #ifndef AST_CONTEXT_HPP_INCLUDED__
