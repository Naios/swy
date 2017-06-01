
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

#ifndef FORMATTING_HPP_INCLUDED__
#define FORMATTING_HPP_INCLUDED__

#include "fmt/format.h"
#include "fmt/ostream.h"

#include "llvm/ADT/StringRef.h"

/// Allows formatting of llvm::StringRef through fmtlib
namespace llvm {
inline std::ostream& operator<<(std::ostream& os, StringRef str) {
  os.write(str.data(), str.size());
  return os;
}
}

/// Promote fmt::format into our namespace
using fmt::format;
using namespace fmt::literals;

#endif // #ifndef FORMATTING_HPP_INCLUDED__
