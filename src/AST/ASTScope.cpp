
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

#include "ASTScope.hpp"

#include <assert.h>
#include <tuple>
#include <unordered_map>

// #include "llvm/ADT/StringMap.h"
#include "Hash.hpp"

#include "AST.hpp"
#include "ASTContext.hpp"

template <typename Current, bool IsConsistent>
class AllocatedScope : public Current {
  llvm::Optional<Current const*> parent_;
  std::unordered_map<llvm::StringRef, NamedDeclContext*, StringRefHasher>
      lookupMap_;

  static unsigned const THRESHOLD = 10U;

public:
  explicit AllocatedScope(llvm::Optional<Current const*> parent)
      : parent_(parent) {
    assert((!parent.hasValue() || *parent) &&
           "Parent isn't allowed to be null!");
  }

  llvm::Optional<ASTScope const*> parent() const override {
    if (parent_)
      return llvm::Optional<ASTScope const*>(*parent_);
    else
      return llvm::Optional<ASTScope const*>();
  }

  void insert(llvm::StringRef str, NamedDeclContext* node) override {
    assert(!lookupMap_.count(str) &&
           "The entry should never exists in the same scope");
    lookupMap_.insert(std::make_pair(str, node));
  }

  Nullable<NamedDeclContext*>
  lookupIdentifier(llvm::StringRef str) const override {
    auto itr = lookupMap_.find(str);
    if (itr != lookupMap_.end())
      return itr->second;
    else if (parent_)
      return (*parent_)->lookupIdentifier(str);
    else
      return nullptr;
  }

  bool isConsistent() const override { return IsConsistent; }

  Nullable<NamedDeclContext*> similarTo(llvm::StringRef str) const override {
    return similarTo(str, THRESHOLD, nullptr);
  }

  Nullable<NamedDeclContext*>
  similarTo(llvm::StringRef str, unsigned distance,
            NamedDeclContext* current) const override {
    // Calculate the local most similar declaration
    {
      NamedDeclContext* myCurrent;
      unsigned myDistance;
      std::tie(myCurrent, myDistance) = myMostSimilarString(str);

      if (myDistance < distance) {
        distance = myDistance;
        current = myCurrent;
      }
    }

    if (parent_)
      return (*parent_)->similarTo(str, distance, current);
    else
      return current;
  }

private:
  std::tuple<NamedDeclContext*, std::size_t>
  myMostSimilarString(llvm::StringRef str) const {
    NamedDeclContext* myMostSimilarContext = nullptr;
    auto myMostSimilarValue = THRESHOLD;

    for (auto const& entry : lookupMap_) {
      auto similarity = str.edit_distance(entry.first, true, THRESHOLD);

      if (similarity < myMostSimilarValue) {
        myMostSimilarContext = entry.second;
        myMostSimilarValue = similarity;
      }
    }

    return std::make_tuple(myMostSimilarContext, myMostSimilarValue);
  }
};

std::unique_ptr<ASTScope>
ASTScope::CreateTemporary(llvm::Optional<ASTScope const*> parent) {
  return std::make_unique<AllocatedScope<ASTScope, false>>(parent);
}

ConsistentASTScope* ConsistentASTScope::CreateConsistent(
    ASTContext* astContext,
    llvm::Optional<ConsistentASTScope const*> parent /*= llvm::None*/) {
  // TODO Also allocate scoped entries through the ASTContext allocator
  return astContext->allocate<AllocatedScope<ConsistentASTScope, true>>(parent);
}

class InplaceASTScopeImpl : public InplaceASTScope {
  ASTScope* parent_;
  ListenerType listener_;

public:
  InplaceASTScopeImpl(ASTScope* parent,
                      llvm::function_ref<void(NamedDeclContext*)> listener)
      : parent_(parent), listener_(listener) {}

  llvm::Optional<ASTScope const*> parent() const override {
    return parent_->parent();
  }
  void insert(llvm::StringRef str, NamedDeclContext* node) override {
    parent_->insert(str, node);
    listener_(node);
  }
  Nullable<NamedDeclContext*>
  lookupIdentifier(llvm::StringRef str) const override {
    return parent_->lookupIdentifier(str);
  }
  bool isConsistent() const override { return parent_->isConsistent(); }
  Nullable<NamedDeclContext*> similarTo(llvm::StringRef str) const override {
    return parent_->similarTo(str);
  }
  Nullable<NamedDeclContext*>
  similarTo(llvm::StringRef str, unsigned distance,
            NamedDeclContext* current) const override {
    return parent_->similarTo(str, distance, current);
  }
};

std::unique_ptr<InplaceASTScope>
InplaceASTScope::CreateInplace(ASTScope* parent, ListenerType listener) {
  return std::make_unique<InplaceASTScopeImpl>(parent, listener);
}
