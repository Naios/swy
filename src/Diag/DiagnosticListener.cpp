
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

#include "DiagnosticListener.hpp"

#include "antlr4-runtime.h"

#include "llvm/Support/SourceMgr.h"

#include "GeneratedLexer.h"
#include "GeneratedParser.h"

#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"

class DiagnosticListener : public antlr4::BaseErrorListener {
  CompilationUnit* compilationUnitInstance_;

public:
  explicit DiagnosticListener(CompilationUnit* compilationUnitInstance)
      : compilationUnitInstance_(compilationUnitInstance) {}

  /// Returns the diagnostic engine which us used for emitting diagnostics
  DiagnosticEngine* diagnosticEngine() const {
    return compilationUnitInstance_->getDiagnosticEngine();
  }

  void syntaxError(antlr4::Recognizer* recognizer,
                   antlr4::Token* /*offendingSymbol*/, size_t line,
                   size_t charPositionInLine, std::string const& msg,
                   std::exception_ptr /*e*/) override {
    auto const location = SourceLocation::fromSource(compilationUnitInstance_,
                                                     line, charPositionInLine);

    // The diagnostic is dispatched when going out of scope
    auto diag = diagnosticEngine()->diagnose(
        Diagnostic::ErrorGenericParserFault, location, msg);

    /// Add lexer specific error information to the diagnostic
    if (dynamic_cast<antlr4::Lexer*>(recognizer)) {
      // Always recommend to delete the offending tokens
      diag.addFixItReplace(location.extend(1), {});
    }
    /// Add parser specific error information to the diagnostic
    else if (auto parser = dynamic_cast<antlr4::Parser*>(recognizer)) {
      // Add a token range if available
      auto size = parser->getCurrentToken()->getText().size();
      if (size > 1) {
        diag.addRange(location.extend(size));
      }

      // Recommend the expected tokens as insertion
      for (auto expected : parser->getExpectedTokens().toList()) {
        auto insertion = parser->getVocabulary().getDisplayName(expected);
        diag.addFixItInsert(location, insertion);
      }
    }
  }
};

std::unique_ptr<antlr4::ANTLRErrorListener>
createDiagnosticListener(CompilationUnit* compilationUnitInstance) {
  return std::make_unique<DiagnosticListener>(compilationUnitInstance);
}
