
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

#include "SourceLocation.hpp"

#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"

SourceRange SourceLocation::extend(size_t extension) const {
  auto upper = llvm::SMLoc::getFromPointer(location_.getPointer() + extension);
  return SourceRange(*this, SourceLocation(upper));
}

SourceLocation
SourceLocation::fromSource(CompilationUnit const* compilationUnit, size_t line,
                           size_t charPositionInLine) {
  // Translates the line number and offset of a token position back to
  // the llvm::SMLoc which takes just the character offset.
  // This method is really evil, however it connects antlr nicely to the llvm
  // diagnostic engine.
  auto id = compilationUnit->getSourceFileId();
  auto buffer =
      compilationUnit->getCompilerInstance()->getSourceMgr().getMemoryBuffer(
          id);

  auto current = buffer->getBufferStart();
  auto end = buffer->getBufferEnd();

  // Extend the character position to the line
  for (; (current < end) && (line != 1); ++current) {
    if (*current == '\n') {
      --line;
    }
  };
  // Extend the character position to the offset
  for (; (current < end); ++current, --charPositionInLine) {
    if (charPositionInLine == 0) {
      return SourceLocation(llvm::SMLoc::getFromPointer(current));
    }
  };
  return SourceLocation(llvm::SMLoc::getFromPointer(buffer->getBufferEnd()));
}
