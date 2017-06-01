
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

#ifndef SOURCE_ANNOTATED_HPP_INCLUDED__
#define SOURCE_ANNOTATED_HPP_INCLUDED__

#include <ostream>
#include <type_traits>

#include "llvm/ADT/StringRef.h"

#include "SourceLocation.hpp"

// namespace detail {
template <typename T> T* pointerize(T& type) { return &type; }
template <typename T> T* pointerize(T* type) { return type; }
// } // end namespace detail

/// A class wrapping types together with an annotation
template <typename Type, typename AnnotationType> class SourceAnnotated {
  Type type_;
  AnnotationType annotation_;

public:
  SourceAnnotated(Type type, AnnotationType annotation)
      : type_(std::move(type)), annotation_(std::move(annotation)) {}

  template <typename T, typename = std::enable_if_t<std::is_convertible<
                            std::decay_t<T>, std::decay_t<Type>>::value>>
  SourceAnnotated(SourceAnnotated<T, AnnotationType> annotated)
      : type_(std::move(annotated.type_)),
        annotation_(std::move(annotated.annotation_)) {}

  /// Returns the type
  Type const& getType() const { return type_; }
  /// Returns the attached annotation of the type
  AnnotationType const& getAnnotation() const { return annotation_; }

  /// Compare annotated types only
  bool operator==(SourceAnnotated const& right) const {
    return type_ == right.type_;
  }
  /// Compare annotated types only
  bool operator!=(SourceAnnotated const& right) const {
    return type_ != right.type_;
  }
  /// Compare annotated types only
  bool operator==(Type const& right) const { return type_ == right; }
  /// Compare annotated types only
  bool operator!=(Type const& right) const { return type_ != right; }

  Type& operator*() { return type_; }
  Type const& operator*() const { return type_; }
  std::remove_pointer_t<Type>* operator->() { return pointerize(type_); }
  std::remove_pointer_t<Type> const* operator->() const {
    return pointerize(type_);
  }
  explicit operator Type() const { return type_; }

  friend std::ostream& operator<<(std::ostream& os,
                                  SourceAnnotated const& annotated) {
    return os << annotated.getType();
  }
};

/// A type which is annotated with a source code location
template <typename T>
using LocationAnnotated = SourceAnnotated<T, SourceLocation>;
/// A type which is annotated with a source code range
template <typename T> using RangeAnnotated = SourceAnnotated<T, SourceRange>;

/// A type which is specialized for identifiers in the source code
using Identifier = RangeAnnotated<llvm::StringRef>;

#endif // #ifndef SOURCE_ANNOTATED_HPP_INCLUDED__
