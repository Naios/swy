
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

#ifndef TOKEN_DUMPER_HPP_INCLUDED__
#define TOKEN_DUMPER_HPP_INCLUDED__

#include "llvm/Support/raw_ostream.h"

namespace antlr4 {
class CommonTokenStream;
class Recognizer;
}

/// Prints the tokens as YAML to the given ostream
void dumpTokens(llvm::raw_ostream& out, antlr4::CommonTokenStream* tokens,
                antlr4::Recognizer const* recognizer);

#endif // #ifndef TOKEN_DUMPER_HPP_INCLUDED__
