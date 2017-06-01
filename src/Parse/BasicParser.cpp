
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

#include "BasicParser.hpp"

#include <cassert>

BasicParser::~BasicParser() {
  assert((Parser::getNumberOfSyntaxErrors() || metaDepthStack_.empty()) &&
         "Ended with an unbalanced depth stack");
}

bool BasicParser::isInMetaDepth(MetaDepth depth) const {
  assert(isInMetaDecl() &&
         "Tried to access the meta depth without being in a meta decl.");
  return metaDepthStack_.top() == depth;
}

void BasicParser::enterDepth(MetaDepth depth) { metaDepthStack_.push(depth); }

void BasicParser::leaveDepth() {
  assert(!metaDepthStack_.empty() &&
         "Tried to pop an unbalanced amount of depth states from the stack!");
  metaDepthStack_.pop();
}
