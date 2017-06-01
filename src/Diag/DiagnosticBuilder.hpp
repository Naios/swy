
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

#ifndef DIAGNOSTIC_BUILDER_HPP_INCLUDED__
#define DIAGNOSTIC_BUILDER_HPP_INCLUDED__

#include <utility>

#include "llvm/ADT/Twine.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"

#include "Diagnostic.hpp"
#include "Ownership.hpp"
#include "SourceLocation.hpp"

class CompilationUnit;
class DiagnosticEngine;

/// Diagnostic builder class to build llvm::SMDiagnostic's
///
/// This class is designed to have a short lifetime.
/// The diagnostic is dispatched on destruction of this class.
class DiagnosticBuilder {
public:
  DiagnosticBuilder(DiagnosticEngine const* diagnosticEngine, Severity severity,
                    SourceLocation location, std::string msg)
      : diagnosticEngine_(diagnosticEngine), severity_(severity),
        location_(location), msg_(std::move(msg)) {}

  ~DiagnosticBuilder();

  DiagnosticBuilder(DiagnosticBuilder const&) = delete;
  DiagnosticBuilder(DiagnosticBuilder&& right) = default;
  DiagnosticBuilder& operator=(DiagnosticBuilder const&) = delete;
  DiagnosticBuilder& operator=(DiagnosticBuilder&&) = delete;

  /// Adds a code range to the diagnostic
  DiagnosticBuilder&& addRange(SourceRange sourceRange);

  /// Adds an insertion fix-it suggestion to the diagnostic
  DiagnosticBuilder&& addFixItInsert(SourceLocation sourceLocation,
                                     llvm::Twine const& insertion);

  /// Adds a replace fix-it suggestion to the diagnostic
  DiagnosticBuilder&& addFixItReplace(SourceRange sourceRange,
                                      llvm::Twine const& replacement);

private:
  DiagnosticEngine const* diagnosticEngine_;
  Severity severity_;
  SourceLocation location_;
  std::string msg_;
  llvm::SmallVector<llvm::SMRange, 2> ranges_;
  llvm::SmallVector<llvm::SMFixIt, 2> fixIts_;
  Ownership ownership_;
};

#endif // #ifndef DIAGNOSTIC_BUILDER_HPP_INCLUDED__
