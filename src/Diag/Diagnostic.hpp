
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

#ifndef DIAGNOSTIC_HPP_INCLUDED__
#define DIAGNOSTIC_HPP_INCLUDED__

/// States the severity of a diagnostic message
enum class Severity : unsigned { Note, Warning, Error, Severity_Max };

/// Represents the diagnostic itself
enum class Diagnostic : unsigned {
#define FOR_EACH_DIAG(SEVERITY, NAME, MESSAGE) DIAG_AS_ENUM(SEVERITY, NAME),
#include "Diagnostic.inl"
};

/// Returns the severity of the given diagnostic
Severity getDiagnosticSeverity(Diagnostic diag);

/// Returns the diagnostic message of the given diagnostic
char const* getDiagnosticMessage(Diagnostic diag);

#endif // #ifndef DIAGNOSTIC_HPP_INCLUDED__
