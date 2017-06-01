
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

#include "NameMangeling.hpp"

#include "AST.hpp"
#include "ASTTraversal.hpp"
#include "Formatting.hpp"

/// TODO make use of llvm::Mangeler when feasible
namespace mangeling {
/// Returns the separator for mangeling a symbol or multiple namespaces
static llvm::StringRef getMangleSeparator() {
  static llvm::StringRef ref = "@";
  return ref;
}

static void writeStringRef(llvm::raw_ostream& ostream, llvm::StringRef ref) {
  ostream.write(ref.begin(), ref.size());
}

void mangleSymbol(llvm::raw_ostream& ostream, llvm::StringRef symbol) {
  auto separator = getMangleSeparator();
  writeStringRef(ostream, separator);
  writeStringRef(ostream, symbol);
  writeStringRef(ostream, separator);
}

static bool mangleUnitEntity(llvm::raw_ostream& ostream, ASTNode const* node);

static bool mangleUnitEntity(llvm::raw_ostream& ostream,
                             MetaUnitASTNode const* node) {
  if (mangleUnitEntity(ostream, node->getContainingUnit())) {
    writeStringRef(ostream, getMangleSeparator());
  }
  mangleNameOf(ostream, node->getInstantiation());
  return true;
}

static bool mangleUnitEntity(llvm::raw_ostream& /*ostream*/,
                             ASTNode const* /*node*/) {
  // Do nothing for nodes where mangeling isn't defined
  return false;
}

static bool doMangleUnitEntity(llvm::raw_ostream& ostream,
                               ASTNode const* node) {
  return traverseNode(node, [&](auto* promoted) {
    return mangleUnitEntity(ostream, promoted);
  });
}

template <typename T>
void mangleTopLevelEntity(llvm::raw_ostream& ostream, T const* node) {
  if (doMangleUnitEntity(ostream, node->getContainingUnit())) {
    writeStringRef(ostream, getMangleSeparator());
  }

  writeStringRef(ostream, *node->getName());
}

void mangleNameOf(llvm::raw_ostream& ostream, MetaUnitASTNode const* node) {
  mangleUnitEntity(ostream, node);
}

void mangleNameOf(llvm::raw_ostream& ostream, FunctionDeclASTNode const* node) {
  mangleTopLevelEntity(ostream, node);
}

void mangleNameOf(llvm::raw_ostream& ostream, MetaDeclASTNode const* node) {
  mangleTopLevelEntity(ostream, node);
}

void mangleNameOf(llvm::raw_ostream& ostream,
                  MetaInstantiationExprASTNode const* node) {
  mangleSymbol(ostream, fmt::format("{}", static_cast<void const*>(node)));
}
} // end namespace mangeling
