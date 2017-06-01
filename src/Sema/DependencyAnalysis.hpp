
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

#ifndef DEPENDENCY_ANALYSIS_HPP_INCLUDED__
#define DEPENDENCY_ANALYSIS_HPP_INCLUDED__

#include "llvm/ADT/STLExtras.h"

class FunctionDeclASTNode;
class MetaDeclASTNode;
class MetaInstantiationExprASTNode;

/// Calls the consumer with all effective MetaInstantiationExprASTNode's
/// of the given node. Return false from the consumer to stop the consumption.
void consumeMetaInstantiationsOf(
    FunctionDeclASTNode const* node,
    llvm::function_ref<bool(MetaInstantiationExprASTNode const*)> consumer);

/// Calls the consumer with all effective MetaInstantiationExprASTNode's
/// of the given node. Return false from the consumer to stop the consumption.
void consumeMetaInstantiationsOf(
    MetaDeclASTNode const* node,
    llvm::function_ref<bool(MetaInstantiationExprASTNode const*)> consumer);

#endif // #ifndef DEPENDENCY_ANALYSIS_HPP_INCLUDED__
