
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

#include "ASTParser.hpp"

#include <memory>

#include "llvm/ADT/Optional.h"

#include "GeneratedParser.h"
#include "GeneratedParserBaseListener.h"

#include "AST.hpp"
#include "ASTDumper.hpp"
#include "ASTLayout.hpp"
#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"
#include "CompilerInvocation.hpp"
#include "DiagnosticListener.hpp"
#include "LocalScopeVisitor.hpp"
#include "SourceLocation.hpp"

llvm::Optional<ASTParseResult>
ASTParser::parse(SharedTokenStream const& tokens) const {
  auto astContext = std::make_shared<ASTContext>();

  auto errorListener = createDiagnosticListener(compilationUnit_);

  GeneratedParser parser(tokens.get());
  parser.removeErrorListeners();
  parser.addErrorListener(errorListener.get());
  // parser.getTokenStream()->mark() and seek
  auto compilationUnit = parser.compilationUnit();
  if (!canContinue()) {
    return llvm::None;
  }

  LocalScopeVisitor localScopeVisitor(compilationUnit_, astContext.get());
  compilationUnit->accept(&localScopeVisitor);

  auto layout = std::move(localScopeVisitor).buildLayout();
  if (!canContinue()) {
    return llvm::None;
  }

  if (compilationUnit_->getCompilerInstance()->getInvocation()->hasEmitAction(
          EmitAction::EmitFlatLayout)) {
    dumpFlatLayout(llvm::outs(), llvm::makeArrayRef(layout));
    return llvm::None;
  }

  if (compilationUnit_->getCompilerInstance()->getInvocation()->hasEmitAction(
          EmitAction::EmitLayout)) {
    dumpLayout(llvm::outs(), llvm::makeArrayRef(layout));
    return llvm::None;
  }

  ASTLayoutReader reader(compilationUnit_, astContext.get(), std::move(layout));
  auto main = reader.consumeCompilationUnit();

  if (!canContinue()) {
    return llvm::None;
  }

  return {{astContext, main}};
}

bool ASTParser::canContinue() const {
  if (compilationUnit_->getDiagnosticEngine()->hasErrors()) {
    auto errors = compilationUnit_->getDiagnosticEngine()->getOccurrenceCount(
        Severity::Error);

    compilationUnit_->getCompilerInstance()->logError(
        "There were {} errors when parsing the source file, aborting!", errors);
    return false;
  }
  return true;
}
