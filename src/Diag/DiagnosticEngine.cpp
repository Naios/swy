
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

#include "llvm/Support/raw_ostream.h"

#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"
#include "DiagnosticEngine.hpp"

void DiagnosticEngine::consumeDiagnostic(
    llvm::SMDiagnostic const& diagnostic) const {
  compilationUnit_->getCompilerInstance()->getSourceMgr().PrintMessage(
      llvm::errs(), diagnostic);

  if (diagnostic.getKind() == llvm::SourceMgr::DK_Error) {
    // ++errorCount;
    /*if (errorCount >= 5) {
      // TODO DO something
      // compilerInstance_->logError("Terminating the compilation because the "
      //                             "error threshold was reached!");
    }*/
  }
}

DiagnosticBuilder
DiagnosticEngine::createDiagnosticBuilder(Diagnostic diag, std::string msg,
                                          SourceLocation location) {
  auto severity = getDiagnosticSeverity(diag);
  ++occurrence_[unsigned(severity)];
  return {this, severity, location, std::move(msg)};
}

DiagnosticBuilder DiagnosticEngine::createDiagnosticBuilder(Diagnostic diag,
                                                            std::string msg,
                                                            SourceRange range) {
  // TODO Dont pass the range with the start point
  return createDiagnosticBuilder(diag, msg, range.getStart()).addRange(range);
}
