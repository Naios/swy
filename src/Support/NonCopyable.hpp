
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

#ifndef NONCOPYABLE_HPP_INCLUDED__
#define NONCOPYABLE_HPP_INCLUDED__

// Class for making child classes non copyable
struct NonCopyable {
  NonCopyable() = default;
  NonCopyable(NonCopyable const&) = delete;
  NonCopyable(NonCopyable&&) = default;
  NonCopyable& operator=(NonCopyable const&) = delete;
  NonCopyable& operator=(NonCopyable&&) = default;
};

// Class for making child classes non copyable and movable
struct NonMovable {
  NonMovable() = default;
  NonMovable(NonMovable const&) = delete;
  NonMovable(NonMovable&&) = delete;
  NonMovable& operator=(NonMovable const&) = delete;
  NonMovable& operator=(NonMovable&&) = delete;
};

#endif // #ifndef NONCOPYABLE_HPP_INCLUDED__
