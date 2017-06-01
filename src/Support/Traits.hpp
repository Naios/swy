
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

#ifndef TRAITS_HPP_INCLUDED__
#define TRAITS_HPP_INCLUDED__

#include <tuple>
#include <type_traits>

namespace traits_detail {
// Equivalent to C++17's std::void_t which targets a bug in GCC,
// that prevents correct SFINAE behavior.
// See http://stackoverflow.com/questions/35753920 for details.
template <typename...> struct deduce_to_void : std::common_type<void> {};
} // end namespace traits_detail

template <typename... T> struct identity : std::tuple<T...> {};
template <typename T> struct identity<T> : std::common_type<T> {};

template <typename T> identity<T> identityOf() { return {}; }
template <typename T> identity<T> identityOf(T /*type*/) { return {}; }
template <typename T> identity<T> identityOf(identity<T> /*type*/) {
  return {};
}

template <typename... T>
using void_t = typename traits_detail::deduce_to_void<T...>::type;

template <typename Predicate, typename FirstArg, typename... Args>
auto firstMatchOf(Predicate&& predicate, ::identity<FirstArg> firstArg,
                  Args&&... args);

namespace traits_detail {
template <typename T, typename Check, typename = void_t<>>
struct is_valid_impl : std::common_type<std::false_type> {};

template <typename T, typename Check>
struct is_valid_impl<T, Check,
                     void_t<decltype(std::declval<Check>()(std::declval<T>()))>>
    : std::common_type<std::true_type> {};

template <typename Type, typename Callback>
void static_if_impl(std::true_type, Type&& type, Callback&& callback) {
  std::forward<Callback>(callback)(std::forward<Type>(type));
}

template <typename Type, typename Callback>
void static_if_impl(std::false_type, Type&& /*type*/, Callback&& /*callback*/) {
}

template <typename Predicate, typename FirstArg, typename... Args>
auto firstMatchOfImpl(std::true_type, Predicate&& /*predicate*/,
                      identity<FirstArg> firstArg, Args&&... /*args*/) {
  return firstArg;
}

template <typename Predicate, typename FirstArg, typename... Args>
auto firstMatchOfImpl(std::false_type, Predicate&& predicate,
                      identity<FirstArg> /*firstArg*/, Args&&... args) {
  return firstMatchOf(std::forward<Predicate>(predicate),
                      std::forward<Args>(args)...);
}
} // end namespace traits_detail

/// A boost::hana like compile-time check for validating a certain expression
template <typename T, typename Check>
auto is_valid(T&& /*type*/, Check&& /*check*/) {
  return typename traits_detail::is_valid_impl<T, Check>::type{};
}

/// An equivalent to boost::hana's hana::is_valid which creates
/// a static functional validator object.
template <typename Check> auto validatorOf(Check&& check) {
  return [check = std::forward<Check>(check)](auto matchable) {
    return is_valid(matchable, check);
  };
}

/// Invokes the callback only if the given type matches the check
template <typename Type, typename Check, typename TrueCallback>
void staticIf(Type&& type, Check&& check, TrueCallback&& trueCallback) {
  traits_detail::static_if_impl(std::forward<Check>(check)(type),
                                std::forward<Type>(type),
                                std::forward<TrueCallback>(trueCallback));
}

/// Invokes the callback only if the given type matches the check
/*template <typename Type, typename Check, typename TrueCallback,
          typename FalseCallback>
void static_if_else(Type&& type, Check&& check, TrueCallback&& trueCallback,
                    FalseCallback&& falseCallback) {
  auto result = std::forward<Check>(check)(type);
  traits_detail::static_if_impl(result, std::forward<Type>(type),
                                std::forward<TrueCallback>(trueCallback));
  traits_detail::static_if_impl(std::integral_constant<bool, !result>{},
                                std::forward<Type>(type),
                                std::forward<FalseCallback>(falseCallback));
}*/

/// Creates a functional object which invokes the given callback when
/// an object is passed to it that passes the check.
template <typename Check, typename Callback>
auto invokeFilter(Check&& check, Callback&& callback) {
  return [
    check = std::forward<Check>(check),
    callback = std::forward<Callback>(callback)
  ](auto matchable) {
    return staticIf(matchable, check, callback);
  };
}

/// Evaluates the given callback with the type only if a true type is given.
/// Otherwise return the default value.
template <typename Type, typename TrueCallback, typename FalseCallback>
auto conditionalEvaluate(std::true_type, Type&& type,
                         TrueCallback&& trueCallback,
                         FalseCallback&& /*falseCallback*/) {
  return std::forward<TrueCallback>(trueCallback)(std::forward<Type>(type));
}

/// Evaluates the given callback with the type only if a true type is given.
/// Otherwise return the default value.
template <typename Type, typename TrueCallback, typename FalseCallback>
auto conditionalEvaluate(std::false_type, Type&& type,
                         TrueCallback&& /*trueCallback*/,
                         FalseCallback&& falseCallback) {
  return std::forward<FalseCallback>(falseCallback)(std::forward<Type>(type));
}

/// Decorates the given functional to always return the given type
template <typename ReturnType, typename T>
auto decorate(identity<ReturnType>, T&& functor) {
  return [functor = std::forward<T>(functor)](auto&& type)->ReturnType {
    return functor(std::forward<decltype(type)>(type));
  };
}

/// Returns a functor which returns the given value
template <typename T> auto supplierOf(T value = {}) {
  return [value = std::move(value)](auto&&...) { return value; };
}

/// Evaluates to the plain type of a qualified one
template <typename T>
using plain_t = std::decay_t<std::remove_pointer_t<std::decay_t<T>>>;

/// Plains the given identity type
template <typename T> auto plainify(identity<T> = {}) {
  return identityOf<plain_t<T>>();
}

/// Evaluates to a true type if the first type is contained
/// in the following arguments
template <typename T> auto isAnyOf(T /*type*/) { return std::false_type{}; }

/// Evaluates to a true type if the first type is contained
/// in the following arguments
template <typename T, typename... Rest>
auto isAnyOf(T /*type*/, T /*first*/, Rest&&... /*rest*/) {
  return std::true_type{};
}

/// Evaluates to a true type if the first type is contained
/// in the following arguments
template <typename T, typename First, typename... Rest>
auto isAnyOf(T type, First /*first*/, Rest&&... rest) {
  return isAnyOf(type, std::forward<Rest>(rest)...);
}

/// Evaluates to the type that first matches the given predicate
/// All arguments must be passed wrapped inside an identity.
///
/// The function expects at least 1 argument to match the predicate.
template <typename Predicate, typename FirstArg, typename... Args>
auto firstMatchOf(Predicate&& predicate, identity<FirstArg> firstArg,
                  Args&&... args) {
  auto result = decltype(predicate(std::declval<FirstArg>())){};
  static_assert(decltype(result)::value || (sizeof...(args) >= 1),
                "No element matched the predicate!");
  return traits_detail::firstMatchOfImpl(
      result, std::forward<Predicate>(predicate), std::move(firstArg),
      std::forward<Args>(args)...);
}

#endif // #ifndef TRAITS_HPP_INCLUDED__
