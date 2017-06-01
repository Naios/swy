
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

#ifndef COMPILER_INVOCATION_HPP_INCLUDED__
#define COMPILER_INVOCATION_HPP_INCLUDED__

#include <bitset>
#include <string>

class CompilerInstance;

/// Represents the selected emit option which makes the frontend
/// to stop on a specific point and print the current result.
enum class EmitAction {
  EmitNone,       ///< Print nothing
  EmitTokens,     ///< Print the result of the Lexer
  EmitFlatLayout, ///< Print the result of the source Parser
  EmitLayout,     ///< Print the structured result of the source parser
  EmitAST         ///< Print the parsed AST
};

/// Represents the optimizations which are applied to runtime code
enum class OptLevel {
  Debug, ///< Perform no optimizations
  O1,    ///< Perform trivial optimizations
  O2,    ///< Perform default optimizations
  O3     ///< Perform expensive optimizations
};

/// Enables debugging messages while performing actions
enum class VerboseFlag {
  All,                ///< Prints all verbose messages
  Shipments,          ///< Prints all shipments to the code executor JIT
  Instantiations,     ///< Prints all meta decl instantiations
  InstantiatedLayout, ///< Prints the AST layout of performed instantiations
  InstantiatedAST,    ///< Prints the parsed AST of performed instantiations
  InstantiatedExports ///< Prints the exported value of instantiations
};

/// Represents the a single compiler invocation
/// Makes global options and command line arguments available for usage
class CompilerInvocation {
  using Bitset = std::bitset<sizeof(unsigned) * 8>;

  EmitAction emitAction_ = EmitAction::EmitNone;
  OptLevel optLevel_ = OptLevel::Debug;
  Bitset verboseFlags_;

  static std::string getDefaultTargetTriple();
  std::string targetTriple_ = getDefaultTargetTriple();

public:
  CompilerInvocation() = default;

  void setEmitAction(EmitAction action) { emitAction_ = action; }
  /// Returns true when the invocation has the given emit action
  bool hasEmitAction(EmitAction action) const { return emitAction_ == action; }

  void setOptLevel(OptLevel optLevel) { optLevel_ = optLevel; }
  /// Returns the optimization level which is set for the invocation
  OptLevel getOptLevel() const { return optLevel_; }

  void setVerboseFlags(Bitset verboseFlags);
  /// Returns true when the invocation shall print the given verbose message
  bool hasVerboseFlag(VerboseFlag verboseFlags) const {
    return verboseFlags_[unsigned(verboseFlags)];
  }

  /// Sets the target triple we are producing code
  void setTargetTriple(std::string targetTriple);
  /// Returns the target triple we are producing code for
  std::string getTargetTriple() const;
};

#endif // #ifndef COMPILER_INVOCATION_HPP_INCLUDED__
