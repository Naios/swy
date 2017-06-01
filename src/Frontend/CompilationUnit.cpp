
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

#include "CompilationUnit.hpp"

#include "antlr4-runtime.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/SourceMgr.h"

#include "GeneratedLexer.h"
#include "GeneratedParser.h"

#include "AST.hpp"
#include "ASTDumper.hpp"
#include "ASTParser.hpp"
#include "CodegenInstance.hpp"
#include "CompilerInstance.hpp"
#include "CompilerInvocation.hpp"
#include "DiagnosticListener.hpp"
#include "SemaAnalysis.hpp"
#include "TokenDumper.hpp"

CompilationUnit::CompilationUnit(CompilerInstance* compilerInstance,
                                 unsigned sourceFileId, std::string filePath)
    : compilerInstance_(compilerInstance), sourceFileId_(sourceFileId),
      filePath_(std::move(filePath)),
      fileName_(llvm::sys::path::filename(filePath_)), diagnosticEngine_(this) {
}

void CompilationUnit::translate() {
  // Create an antlr input stream from the llvm buffer containing
  // the source file
  auto buffer =
      getCompilerInstance()->getSourceMgr().getMemoryBuffer(sourceFileId_);
  antlr4::ANTLRInputStream input(buffer->getBuffer());

  // Instantiate the lexer and register the custom error handler
  GeneratedLexer lexer(&input);
  auto errorListener = createDiagnosticListener(this);
  lexer.removeErrorListeners();
  lexer.addErrorListener(errorListener.get());

  auto tokens = std::make_shared<antlr4::CommonTokenStream>(&lexer);

  if (getCompilerInstance()->getInvocation()->hasEmitAction(
          EmitAction::EmitTokens)) {
    tokens->fill();

    if (diagnosticEngine_.hasErrors()) {
      getCompilerInstance()->logError(
          "There were {} errors when lexing the source file, aborting!",
          diagnosticEngine_.getOccurrenceCount(Severity::Error));
      return;
    }

    dumpTokens(llvm::outs(), tokens.get(), &lexer);
    return;
  }

  ASTParser generator(this);

  auto result = generator.parse(tokens);

  if (!result)
    return;

  SemaAnalysis semaAnalysis(this, result->getCompilationUnit());
  semaAnalysis.checkAST();
  if (diagnosticEngine_.hasErrors()) {
    getCompilerInstance()->logError(
        "There were {} errors when checking the source file "
        "for semantical correctness, aborting!",
        diagnosticEngine_.getOccurrenceCount(Severity::Error));
    return;
  }

  if (getCompilerInstance()->getInvocation()->hasEmitAction(
          EmitAction::EmitAST)) {
    dumpAST(llvm::outs(), result->getCompilationUnit());
    return;
  }

  auto codegen =
      CodegenInstance::createFor(this, result->getASTContext().get());

  if (!codegen) {
    return;
  }

  if (codegen->codegen(result->getCompilationUnit())) {
    codegen->dump();
  }

  // emitAST(getCompilerInstance(), tree);
}
