
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

#ifndef SOURCE_LOCATION_RESOLVER_HPP_INCLUDED__
#define SOURCE_LOCATION_RESOLVER_HPP_INCLUDED__

// FIXME Just an idea, implement later

#include <string>

#include "Diagnostic.hpp"
#include "DiagnosticBuilder.hpp"
#include "SourceLocation.hpp"

/// Is able to resolve a certain SourceLocation to it's original location
/// in the source code while also displaying involved meta-computations.
class SourceLocationResolver {
public:
  virtual ~SourceLocationResolver() = default;

  /// Returns a diagnostic DiagnosticBuilder which emits a diagnostic
  /// related to the given SourceLocation.
  virtual DiagnosticBuilder diagnose(SourceLocation location, Severity severity,
                                     std::string message) = 0;

  /// Returns a diagnostic DiagnosticBuilder which emits a diagnostic
  /// related to the given SourceRange.
  virtual DiagnosticBuilder diagnose(SourceRange range, Severity severity,
                                     std::string message) = 0;
};

#endif // #ifndef SOURCE_LOCATION_RESOLVER_HPP_INCLUDED__
