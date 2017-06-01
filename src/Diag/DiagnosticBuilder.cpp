
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

#include "DiagnosticBuilder.hpp"

#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"
#include "DiagnosticEngine.hpp"

static llvm::SourceMgr::DiagKind mapSeverityToDiagKind(Severity severity) {
  switch (severity) {
    case Severity::Note:
      return llvm::SourceMgr::DiagKind::DK_Note;
    case Severity::Warning:
      return llvm::SourceMgr::DiagKind::DK_Warning;
    case Severity::Error:
      return llvm::SourceMgr::DiagKind::DK_Error;
    default:
      llvm_unreachable("There was no mapping available for this severity!");
  }
}

DiagnosticBuilder::~DiagnosticBuilder() {
  if (!ownership_.hasOwnership())
    return;

  assert(location_.toLLVMLocation().isValid() &&
         "Given diagnostic has an invalid location!");

  // Build the diagnostic
  auto kind = mapSeverityToDiagKind(severity_);

  auto message = diagnosticEngine_->getCompilationUnit()
                     ->getCompilerInstance()
                     ->getSourceMgr()
                     .GetMessage(location_.toLLVMLocation(), kind,
                                 llvm::Twine(msg_), ranges_, fixIts_);

  diagnosticEngine_->consumeDiagnostic(message);
}

DiagnosticBuilder&& DiagnosticBuilder::addRange(SourceRange sourceRange) {
  ranges_.emplace_back(sourceRange.toLLVMRange());
  return std::move(*this);
}

DiagnosticBuilder&&
DiagnosticBuilder::addFixItInsert(SourceLocation sourceLocation,
                                  llvm::Twine const& insertion) {
  fixIts_.push_back({sourceLocation.toLLVMLocation(), insertion});
  return std::move(*this);
}

DiagnosticBuilder&&
DiagnosticBuilder::addFixItReplace(SourceRange sourceRange,
                                   llvm::Twine const& replacement) {
  fixIts_.push_back({sourceRange.toLLVMRange(), replacement});
  return std::move(*this);
}
