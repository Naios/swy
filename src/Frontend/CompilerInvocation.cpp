
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

#include "CompilerInvocation.hpp"

#include <utility>

#include "llvm/Support/Host.h"

void CompilerInvocation::setVerboseFlags(Bitset verboseFlags) {
  verboseFlags_ = verboseFlags;
  if (verboseFlags_[unsigned(VerboseFlag::All)]) {
    verboseFlags_.reset();
    verboseFlags_.flip();
  }
}

void CompilerInvocation::setTargetTriple(std::string targetTriple) {
  targetTriple_ = std::move(targetTriple);
}

std::string CompilerInvocation::getTargetTriple() const {
  return targetTriple_;
}

std::string CompilerInvocation::getDefaultTargetTriple() {
  return llvm::sys::getDefaultTargetTriple();
}
