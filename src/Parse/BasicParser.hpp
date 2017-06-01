
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

#ifndef BASIC_PARSER_HPP_INCLUDED__
#define BASIC_PARSER_HPP_INCLUDED__

#include <stack>

#include "Parser.h"
#include "TokenStream.h"

/// Describes the depth of the current non terminal, which conditional enables
/// nodes as possible children.
enum class MetaDepth {
  DepthGlobalScope, /// Top level nodes like functions or meta decls are allowed
  DepthLocalScope,  /// Locale nodes like statements are allowed
  DepthNone         /// No meta non terminals are allowed
};

/// Custom super class adapting the ANTLR parser without modifying
/// the grammar file.
class BasicParser : public antlr4::Parser {
  std::stack<MetaDepth> metaDepthStack_;

public:
  explicit BasicParser(antlr4::TokenStream* input) : Parser(input) {}
  virtual ~BasicParser();

  /// Returns true when we are in a meta decl
  bool isInMetaDecl() const { return !metaDepthStack_.empty(); }

  /// Returns true when the parser is in the given depth of a meta decl.
  bool isInMetaDepth(MetaDepth depth) const;
  /// Enters the given meta depth
  void enterDepth(MetaDepth depth);
  /// Pops the top state from the stack
  void leaveDepth();

private:
  /// Returns true when the parser is in the given depth of a meta decl or
  /// deeper
  bool isInMetaDepthAsDeepAs(MetaDepth depth) const {
    return metaDepthStack_.top() >= depth;
  }
};

#endif // #ifndef BASIC_PARSER_HPP_INCLUDED__
