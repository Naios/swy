
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

#ifndef COMPILATION_UNIT_HPP_INCLUDED__
#define COMPILATION_UNIT_HPP_INCLUDED__

#include <string>
#include <utility>

#include "DiagnosticEngine.hpp"

namespace antlr4 {
class Token;
}

namespace llvm {
class SMDiagnostic;
}

class CompilerInstance;

/// Represents a compilation unit
class CompilationUnit {
  CompilerInstance* compilerInstance_;
  unsigned sourceFileId_;
  std::string filePath_;
  llvm::StringRef fileName_;
  DiagnosticEngine diagnosticEngine_;

public:
  CompilationUnit(CompilerInstance* compilerInstance, unsigned sourceFileId,
                  std::string filePath);

  /// Returns the compiler instance that owns this compilation unit
  CompilerInstance* getCompilerInstance() const { return compilerInstance_; }
  /// Returns the compiler instance that is owned by this compilation unit
  DiagnosticEngine* getDiagnosticEngine() { return &diagnosticEngine_; }
  /// Returns the source file id that represents a file in the llvm::SourceMgr
  unsigned getSourceFileId() const { return sourceFileId_; }
  /// Returns the path to the source file
  llvm::StringRef getSourceFilePath() const { return filePath_; }
  /// Returns the name of the source file
  llvm::StringRef getSourceFileName() const { return fileName_; }

  /// Translates the given translation unit
  void translate();
};

#endif // #ifndef COMPILATION_UNIT_HPP_INCLUDED__
