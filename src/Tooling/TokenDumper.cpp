
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

#include "TokenDumper.hpp"

#include <cstdint>
#include <vector>

#include "llvm/ObjectYAML/YAML.h"

#include "CommonTokenStream.h"
#include "Recognizer.h"
#include "Token.h"
#include "Vocabulary.h"

struct SimpleToken {
  std::size_t line;
  std::size_t offset;
  std::string kind;
  std::string text;

  static SimpleToken createFrom(antlr4::Recognizer const* recognizer,
                                antlr4::Token* token) {
    SimpleToken simpleToken;
    simpleToken.line = token->getLine();
    simpleToken.offset = token->getCharPositionInLine();
    simpleToken.kind =
        recognizer->getVocabulary().getSymbolicName(token->getType());
    simpleToken.text = token->getText();
    return simpleToken;
  }
};

template <> struct llvm::yaml::MappingTraits<SimpleToken> {
  static void mapping(llvm::yaml::IO& io, SimpleToken& token) {
    io.mapRequired("kind", token.kind);
    io.mapRequired("text", token.text);
    io.mapRequired("line", token.line);
    io.mapRequired("offset", token.offset);
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(SimpleToken)

void dumpTokens(llvm::raw_ostream& out, antlr4::CommonTokenStream* tokens,
                antlr4::Recognizer const* recognizer) {
  tokens->fill();
  auto allTokens = tokens->getTokens();
  allTokens.pop_back(); // Pop the EOF Token

  std::vector<SimpleToken> simpleTokens;
  for (auto token : allTokens) {
    simpleTokens.emplace_back(SimpleToken::createFrom(recognizer, token));
  }

  llvm::yaml::Output yout(out);
  yout << simpleTokens;
}
