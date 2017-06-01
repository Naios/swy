
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

#ifndef HASH_HPP_INCLUDED__
#define HASH_HPP_INCLUDED__

#include "llvm/ADT/Hashing.h"

class StringRefHasher {
public:
  std::size_t operator()(llvm::StringRef str) const {
    return llvm::hash_value(str);
  }
};

class PointerUnionHasher {
public:
  template <typename T> std::size_t operator()(T const& pointerUnion) const {
    return std::hash<void const*>{}(pointerUnion.getOpaqueValue());
  }
};

class PointerUnionEquality {
public:
  template <typename T>
  std::size_t operator()(T const& left, T const& right) const {
    return left.getOpaqueValue() == right.getOpaqueValue();
  }
};

#endif // #ifndef HASH_HPP_INCLUDED__
