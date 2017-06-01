
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

#include "ASTContext.hpp"

#include "AST.hpp"

ASTContext::~ASTContext() {
  // Call the destructor for all allocated AST nodes
  for (auto allocated : allocated_) {
    allocated->~ASTFragment();
  }
}

llvm::StringRef ASTContext::poolString(llvm::StringRef str) {
  auto itr = stringPool_.find(str);
  if (itr != stringPool_.end()) {
    return *itr;
  }
  auto const size = str.size();
  auto* rep = static_cast<char*>(allocator_.Allocate(size, alignof(char)));

  for (std::size_t i = 0; i < size; ++i) {
    rep[i] = str[i];
  }
  llvm::StringRef ref(rep, size);
  stringPool_.insert(ref);
  return ref;
}
