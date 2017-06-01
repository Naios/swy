
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

#ifndef NULLABLE_HPP_INCLUDED__
#define NULLABLE_HPP_INCLUDED__

#include <cassert>
#include <iterator>
#include <type_traits>

template <typename> class Nullable;

/// Makes a Nullable from the given type
template <typename T> Nullable<T> makeNullable(Nullable<T> nullable) {
  return nullable;
}
/// Makes a Nullable from the given type
template <typename T> Nullable<T*> makeNullable(T* nullable) {
  return nullable;
}

/// Asserts if the given pointer is null
template <typename T> T* requiresNonNull(T* ptr) {
  assert(ptr && "Require the pointer to be non null!");
  return ptr;
}

/// An optional type specialized to wrap nullable pointers
///
/// This class has some advantages over llvm::Optional because it uses
/// nullptr as obvious absent state and returns a reference to the object
/// directly when calling the operator->.
/// This makes the Nullable a near drop-in replacement for pointers
/// in order to catch misusage and null dereferencing early.
///
/// The nullable also supports the container requirements
/// for supporting possible one child iterations.
template <typename T> class Nullable {
  template <typename> friend class Nullable;

  static_assert(std::is_pointer<T>::value, "Expected a pointer!");

  T ptr_ = nullptr;

public:
  using iterator = T*;
  using const_iterator = T const*;

  Nullable() = default;
  template <typename O,
            typename = std::enable_if_t<std::is_convertible<O, T>::value>>
  Nullable(O ptr) : ptr_(ptr) {}
  template <typename O,
            typename = std::enable_if_t<std::is_convertible<O, T>::value>>
  Nullable(Nullable<O> right) : ptr_(right.ptr_) {}

  /// Returns true when the nullable is empty
  bool empty() const { return ptr_ == nullptr; }

  /// Returns true when the operator isn't null
  explicit operator bool() const { return !empty(); }
  /// Is convertible to the pointer
  explicit operator T() const { return ptr_; }

  /// Returns the beginning of the nullable
  iterator begin() { return &ptr_; }
  /// Returns the beginning of the nullable
  const_iterator begin() const { return &ptr_; }
  /// Returns the end of the nullable
  iterator end() { return !empty() ? std::next(begin()) : begin(); }
  /// Returns the end of the nullable
  const_iterator end() const { return !empty() ? std::next(begin()) : begin(); }

  /// Returns the pointer of the nullable while asserting that it isn't null
  T operator->() const { return requiresNonNull(ptr_); }
  /// Returns the pointer of the nullable while asserting that it isn't null
  T operator*() const { return requiresNonNull(ptr_); }

  /// Maps the Nullable with the given function to a plain type or
  /// to a new Nullable which can be absent again.
  template <typename F>
  auto map(F&& fn) -> decltype(makeNullable(std::forward<F>(fn)(ptr_))) {
    if (!empty()) {
      return std::forward<F>(fn)(ptr_);
    } else {
      return {};
    }
  }
};

/// Casts the nullable to the given type
template <typename T, typename A> T static_nullable_cast(Nullable<A> nullable) {
  if (nullable)
    return static_cast<T>(*nullable);
  else
    return nullptr;
}

/// Expect the pointer never to be null and initialized on use.
/// This is different from nullable since it doesn't default to a nullptr
/// state in release mode and the class doesn't provide methods for checking
/// the pointer for existence.
template <typename T> class NonNull {
  template <typename> friend class NonNull;

  static_assert(std::is_pointer<T>::value, "Expected a pointer!");

#ifndef NDEBUG
  T ptr_ = nullptr;
#else
  T ptr_;
#endif

public:
  NonNull() = default;
  template <typename O,
            typename = std::enable_if_t<std::is_convertible<O, T>::value>>
  NonNull(O ptr) : ptr_(requiresNonNull(ptr)) {
    static_assert(!std::is_null_pointer<O>::value, "Can't accept a nullptr!");
  }
  template <typename O,
            typename = std::enable_if_t<std::is_convertible<O, T>::value>>
  NonNull(NonNull<O> right) : ptr_(requiresNonNull(right.ptr_)) {}

  /// Returns the pointer
  T operator->() const { return requiresNonNull(ptr_); }
  /// Returns the pointer
  T operator*() const { return requiresNonNull(ptr_); }
};

#endif // #ifndef NULLABLE_HPP_INCLUDED__
