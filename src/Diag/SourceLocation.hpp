
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

#ifndef SOURCE_LOCATION_HPP_INCLUDED__
#define SOURCE_LOCATION_HPP_INCLUDED__

#include "llvm/Support/SMLoc.h"
#include <cstddef>

class CompilationUnit;
class SourceRange;

/// Represents a location in the source file
class SourceLocation {
  llvm::SMLoc location_;

public:
  explicit SourceLocation(llvm::SMLoc location) : location_(location) {}

  /// Extends the given code location to a range where the extension
  /// represents the final size of the range.
  SourceRange extend(std::size_t extension) const;

  /// Returns a SourceLocation constructed from the character
  /// position in the given compilation unit.
  static SourceLocation fromSource(CompilationUnit const* compilationUnit,
                                   std::size_t line, size_t charPositionInLine);

  /// Returns the location as llvm::SMLoc
  llvm::SMLoc toLLVMLocation() const { return location_; }
};

/// Represents a range in the associated source file
class SourceRange {
  SourceLocation start_;
  SourceLocation end_;

public:
  SourceRange(SourceLocation start, SourceLocation end)
      : start_(start), end_(end) {}

  /// Returns the start of the SourceRange
  SourceLocation getStart() const { return start_; }
  /// Returns the end of the SourceRange
  SourceLocation getEnd() const { return end_; }

  /// Returns the location as llvm::SMRange
  llvm::SMRange toLLVMRange() const {
    return {start_.toLLVMLocation(), end_.toLLVMLocation()};
  }

  /// Merges the start and the end of both source ranges together into one range
  static SourceRange concat(SourceRange const& begin, SourceRange const& end) {
    return {begin.getStart(), end.getEnd()};
  }
};

#endif // #ifndef SOURCE_LOCATION_HPP_INCLUDED__
