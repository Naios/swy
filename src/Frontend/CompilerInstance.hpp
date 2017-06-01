
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

#ifndef COMPILER_INSTANCE_HPP_INCLUDED__
#define COMPILER_INSTANCE_HPP_INCLUDED__

#include <memory>

#include "llvm/Support/SourceMgr.h"

#include "CompilerInvocation.hpp"
#include "Diagnostic.hpp"
#include "Formatting.hpp"

namespace llvm {
class TargetMachine;
}

/// Represents an instance of the compiler
class CompilerInstance {
  CompilerInvocation compilerInvocation_;
  llvm::SourceMgr sourceMgr;
  llvm::TargetMachine const* targetMachine_;
  llvm::TargetMachine const* hostMachine_;

  explicit CompilerInstance(CompilerInvocation const& compilerInvocation,
                            llvm::TargetMachine const* targetMachine,
                            llvm::TargetMachine const* hostMachine)
      : compilerInvocation_(std::move(compilerInvocation)),
        targetMachine_(targetMachine), hostMachine_(hostMachine) {}

public:
  /// Creates and initializes a compiler instance with the given
  /// CompilerInvocation. The result might be empty.
  static std::unique_ptr<CompilerInstance>
  create(CompilerInvocation const& compilerInvocation);

  /// Returns the compiler invocation which contains runtime options
  CompilerInvocation const* getInvocation() const {
    return &compilerInvocation_;
  }

  llvm::SourceMgr const& getSourceMgr() const { return sourceMgr; }

  llvm::ErrorOr<bool /*todo*/> compileSourceFile(std::string path);

  /// Returns the target machine we are generating code for
  llvm::TargetMachine const* getTargetMachine() const { return targetMachine_; }
  /// Returns the host machine the compiler is running on
  llvm::TargetMachine const* getHostMachine() const { return hostMachine_; }

  template <typename... Args>
  void logInfo(llvm::StringRef msg, Args&&... args) {
    logSeverity(Severity::Note, msg, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void logWarning(llvm::StringRef msg, Args&&... args) {
    logSeverity(Severity::Warning, msg, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void logError(llvm::StringRef msg, Args&&... args) {
    logSeverity(Severity::Error, msg, std::forward<Args>(args)...);
  }

private:
  template <typename... Args>
  void logSeverity(Severity severity, llvm::StringRef msg, Args&&... args) {
    logSeverity(severity,
                format(msg.str() /*FIXME*/, std::forward<Args>(args)...));
  }
  void logSeverity(Severity severity, llvm::StringRef msg);
};

#endif // #ifndef COMPILER_INSTANCE_HPP_INCLUDED__
