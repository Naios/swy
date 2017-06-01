
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

#ifndef NAME_MANGELING_HPP_INCLUDED__
#define NAME_MANGELING_HPP_INCLUDED__

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

class MetaUnitASTNode;
class FunctionDeclASTNode;
class MetaDeclASTNode;
class MetaInstantiationExprASTNode;

/// Provides helper functions to mangle the name of various ASTNode's
/// and symbols so it can be distinguished from user defined identifiers.
namespace mangeling {
/// Writes the mangled symbol to the stream
/// which can be distinguished from user defined identifiers.
void mangleSymbol(llvm::raw_ostream& ostream, llvm::StringRef symbol);

/// Writes the mangled name of the node to the given stream
void mangleNameOf(llvm::raw_ostream& ostream, MetaUnitASTNode const* node);
/// Writes the mangled name of the node to the given stream
void mangleNameOf(llvm::raw_ostream& ostream, FunctionDeclASTNode const* node);
/// Writes the mangled name of the node to the given stream
void mangleNameOf(llvm::raw_ostream& ostream, MetaDeclASTNode const* node);
/// Writes the mangled name of the node to the given stream
void mangleNameOf(llvm::raw_ostream& ostream,
                  MetaInstantiationExprASTNode const* node);
} // end namespace mangeling

#endif // #ifndef NAME_MANGELING_HPP_INCLUDED__
