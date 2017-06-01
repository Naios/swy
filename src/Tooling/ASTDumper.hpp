
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

#ifndef AST_DUMPER_HPP_INCLUDED__
#define AST_DUMPER_HPP_INCLUDED__

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/raw_ostream.h"

#include "Nullable.hpp"

class ASTNode;

/// Prints the flat layout (unstructured) as YAML to the given ostream
void dumpFlatLayout(llvm::raw_ostream& out,
                    llvm::ArrayRef<Nullable<ASTNode*>> layout);

/// Prints the flat layout as YAML to the given ostream
void dumpLayout(llvm::raw_ostream& out,
                llvm::ArrayRef<Nullable<ASTNode*>> layout);

/// Prints the complete AST as YAML to the given ostream
void dumpAST(llvm::raw_ostream& out, ASTNode const* astNode);

#endif // #ifndef AST_DUMPER_HPP_INCLUDED__
