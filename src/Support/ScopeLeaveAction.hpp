
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

#ifndef SCOPE_LEAVE_ACTION_HPP_INCLUDED__
#define SCOPE_LEAVE_ACTION_HPP_INCLUDED__

#include "function2/function2.hpp"

#include "Ownership.hpp"

/// Invokes the given function when the class is destroyed
class ScopeLeaveAction {
  using LeaveActionType = fu2::unique_function<void()>;

  LeaveActionType leaveAction_;
  Ownership ownership_;

public:
  explicit ScopeLeaveAction(LeaveActionType leaveAction)
      : leaveAction_(std::move(leaveAction)) {}
  ~ScopeLeaveAction() {
    if (ownership_.hasOwnership()) {
      std::move(leaveAction_)();
    }
  }

  ScopeLeaveAction(ScopeLeaveAction const&) = delete;
  ScopeLeaveAction(ScopeLeaveAction&&) = default;
  ScopeLeaveAction& operator=(ScopeLeaveAction const&) = delete;
  ScopeLeaveAction& operator=(ScopeLeaveAction&&) = default;
};

#endif // #ifndef SCOPE_LEAVE_ACTION_HPP_INCLUDED__
