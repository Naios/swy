
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

#ifndef DIAGNOSTIC_ENGINE_HPP_INCLUDED__
#define DIAGNOSTIC_ENGINE_HPP_INCLUDED__

#include <array>
#include <type_traits>

#include "Diagnostic.hpp"
#include "DiagnosticBuilder.hpp"
#include "Formatting.hpp"
#include "SourceAnnotated.hpp"
#include "SourceLocation.hpp"

class CompilationUnit;

/// Provides the facility for emitting diagnostics about the source code
class DiagnosticEngine {
  friend class DiagnosticBuilder;

  CompilationUnit const* compilationUnit_;
  std::array<std::size_t, std::size_t(Severity::Severity_Max)> occurrence_;

  /// Compile-time check for not passing pointers as diagnostic message arg.
  template <typename T, typename = std::enable_if_t<!std::is_pointer<T>::value>>
  static auto&& filterPointer(T&& t) {
    return std::forward<T>(t);
  }

public:
  explicit DiagnosticEngine(CompilationUnit const* compilationUnit)
      : compilationUnit_(compilationUnit), occurrence_() {}

  /// Returns the compilation unit associated to this DiagnosticEngine
  CompilationUnit const* getCompilationUnit() const { return compilationUnit_; }

  /// Returns a DiagnosticBuilder which can be used to display messages
  DiagnosticBuilder diagnose(Diagnostic diag, SourceLocation location) {
    return createDiagnosticBuilder(diag, getDiagnosticMessage(diag), location);
  }
  /// Returns a DiagnosticBuilder which can be used to display messages
  template <typename... Args>
  DiagnosticBuilder diagnose(Diagnostic diag, SourceLocation location,
                             Args&&... args) {
    auto msg = getDiagnosticMessage(diag);
    return createDiagnosticBuilder(
        diag, fmt::format(msg, std::forward<Args>(filterPointer(args))...),
        location);
  }
  /// Returns a DiagnosticBuilder which can be used to display messages
  DiagnosticBuilder diagnose(Diagnostic diag, SourceRange range) {
    return createDiagnosticBuilder(diag, getDiagnosticMessage(diag), range);
  }
  /// Returns a DiagnosticBuilder which can be used to display messages
  template <typename... Args>
  DiagnosticBuilder diagnose(Diagnostic diag, SourceRange range,
                             Args&&... args) {
    auto msg = getDiagnosticMessage(diag);
    return createDiagnosticBuilder(
        diag, fmt::format(msg, std::forward<Args>(filterPointer(args))...),
        range);
  }
  /// Returns a DiagnosticBuilder which can be used to display messages
  template <
      typename T, typename A, typename... Args,
      typename = std::enable_if_t<std::is_same<A, SourceLocation>::value ||
                                  std::is_same<A, SourceRange>::value>>
  DiagnosticBuilder diagnose(Diagnostic diag,
                             SourceAnnotated<T, A> const& annotation,
                             Args&&... args) {
    return diagnose(diag, annotation.getAnnotation(),
                    std::forward<Args>(args)...);
  }

  /// Returns the count of how often a certain severity was emitted
  std::size_t getOccurrenceCount(Severity severity) const {
    return occurrence_[unsigned(severity)];
  }
  /// Returns true when there were errors emitted
  std::size_t hasErrors() const {
    return getOccurrenceCount(Severity::Error) != 0;
  }

private:
  /// Consumes and displays the given diagnostic
  void consumeDiagnostic(llvm::SMDiagnostic const& diagnostic) const;

  DiagnosticBuilder createDiagnosticBuilder(Diagnostic diag, std::string msg,
                                            SourceLocation location);

  DiagnosticBuilder createDiagnosticBuilder(Diagnostic diag, std::string msg,
                                            SourceRange range);
};

#endif // #ifndef DIAGNOSTIC_ENGINE_HPP_INCLUDED__
