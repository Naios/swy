
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

#include "DependencyAnalysis.hpp"

#include "llvm/Support/ErrorHandling.h"

#include "AST.hpp"
#include "ASTPredicate.hpp"
#include "ASTTraversal.hpp"

using Consumer = llvm::function_ref<bool(MetaInstantiationExprASTNode const*)>;

struct InstConsumer {
  static bool applyConsumer(MetaInstantiationExprASTNode const* node,
                            Consumer const& consumer) {
    return consumer(node);
  }

  static bool applyConsumer(ASTNode const* /*node*/,
                            Consumer const& /*consumer*/) {
    return true;
  }

  static bool consumeAll(ASTNode const* node, Consumer const& consumer) {
    return traverseNode(node, [&consumer](auto promoted) {
      if (!applyConsumer(promoted, consumer)) {
        return false;
      }

      auto hasChildren = pred::hasChildren()(promoted);
      return conditionalEvaluate(hasChildren, promoted,
                                 [&consumer](auto promoted) {
                                   for (auto child : promoted->children()) {
                                     if (!consumeAll(child, consumer)) {
                                       return false;
                                     }
                                   }
                                   return true;
                                 },
                                 supplierOf(true));
    });
  }

  static bool consumeMeta(MetaIfStmtASTNode const* node,
                          Consumer const& consumer) {
    return consumeAll(node->getExpression(), consumer);
  }

  static bool consumeMeta(MetaCalculationStmtASTNode const* node,
                          Consumer const& consumer) {
    return consumeAll(node, consumer);
  }

  static bool consumeMeta(ASTNode const* node, Consumer const& consumer) {
    return consumeAllMeta(node, consumer);
  }

  static bool consumeAllMeta(ASTNode const* node, Consumer const& consumer) {
    return traverseNode(node, [&consumer](auto promoted) {
      auto hasChildren = pred::hasChildren()(promoted);
      return conditionalEvaluate(hasChildren, promoted,
                                 [&consumer](auto promoted) {
                                   for (auto child : promoted->children()) {
                                     auto traversor = [&consumer](auto child) {
                                       return consumeMeta(child, consumer);
                                     };

                                     if (!traverseNode(child, traversor)) {
                                       return false;
                                     }
                                   }
                                   return true;
                                 },
                                 supplierOf(true));
    });
  }
};

void consumeMetaInstantiationsOf(FunctionDeclASTNode const* node,
                                 Consumer consumer) {
  InstConsumer::consumeAll(node, consumer);
}

void consumeMetaInstantiationsOf(MetaDeclASTNode const* node,
                                 Consumer consumer) {
  InstConsumer::consumeAllMeta(node, consumer);
}
