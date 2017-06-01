
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

#include "MetaCodegen.hpp"

#include <type_traits>

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/ErrorHandling.h"

#include "AST.hpp"
#include "ASTContext.hpp"
#include "ASTLayout.hpp"
#include "ASTStringer.hpp"
#include "ASTTraversal.hpp"
#include "Formatting.hpp"
#include "IRContext.hpp"

MetaCodegen::MetaCodegen(IRContext* context, llvm::Function* function)
    : IRContextReplication(context), functionCodegen_(context, function),
      function_(function),
      cursor(DepthLevel::TopLevel, MetaDepthLevel::InsideMetaDecl) {}

void MetaCodegen::codegen(MetaDeclASTNode const* metaDecl) {

  assert(functionCodegen_.getModule() == getModule() &&
         "Expected to code in the same module like the FunctionCodegen!");

  FunctionCodegen::ValueMap::ScopeTy scope(functionCodegen_.values_);
  auto block =
      functionCodegen_.setUpStackFrame(metaDecl->getArgDeclList(), false, true);

  // Export the instantiation parameters as constants
  for (auto arg : metaDecl->getArgDeclList()->children()) {
    if (auto namedArg = llvm::dyn_cast<NamedArgumentDeclASTNode>(arg)) {
      auto value = functionCodegen_.lookupLocal(namedArg);
      createIntroduceNode(namedArg, value);
    }
  }

  auto current = codegenMeta(block, metaDecl->getContribution());
  assert(current && "Always expect a valid control flow here!");
  (void)current;

  assert(current->getModule() == getModule() &&
         "Expected to codegen in the same module like the context!");

  functionCodegen_.builder_.CreateRetVoid();
}

/// Returns true when the given node is marked as an IntermediateNode
static bool isNodeIntermediate(ASTNode const* node) {
  return traverseNode(
      node, decorate(identityOf<bool>(), pred::isBaseOf<IntermediateNode>()));
}

Nullable<llvm::BasicBlock*>
MetaCodegen::codegenMeta(llvm::BasicBlock* /*block*/,
                         MetaDeclASTNode const* /*node*/) {
  llvm_unreachable("The meta decl shouldn't be here!");
}

Nullable<llvm::BasicBlock*>
MetaCodegen::codegenMeta(llvm::BasicBlock* block,
                         MetaContributionASTNode const* node) {

  return codegenChildrenContribution(block, node);
}

Nullable<llvm::BasicBlock*>
MetaCodegen::codegenMeta(llvm::BasicBlock* block,
                         MetaIfStmtASTNode const* node) {

  auto codegenTrue = [=](llvm::BasicBlock* current) {
    return codegenMeta(current, node->getTrueBranch());
  };

  if (node->getFalseBranch()) {
    auto codegenFalse = [=](llvm::BasicBlock* current) {
      return codegenMeta(current, *node->getFalseBranch());
    };
    return functionCodegen_.codegenIfStructure(
        block, node->getExpression(), codegenTrue,
        FunctionCodegen::CodegenSupplier(codegenFalse));

  } else {
    return functionCodegen_.codegenIfStructure(block, node->getExpression(),
                                               codegenTrue);
  }
}

Nullable<llvm::BasicBlock*>
MetaCodegen::codegenMeta(llvm::BasicBlock* block,
                         MetaCalculationStmtASTNode const* node) {
  /// Leave this here for ensuring no regressions
  assert(block->getModule() == getModule());

  block = functionCodegen_.createRegionAfter(block, "meta_calculation");
  auto current = functionCodegen_.codegenStmt(block, node->getStmt());

  /// Leave this here for ensuring no regressions
  assert(current && (current->getModule() == getModule()) &&
         "Expected to codegen in the same module like the context!");

  for (auto exported : node->getExportedDecls()) {
    // Introduce the scope of the calculation into the new AST.
    // How this happens isn't important for us here,
    // we just call the callback with a pointer to the value.
    createIntroduceNode(exported, functionCodegen_.lookupLocal(exported));
  }

  return current;
}

Nullable<llvm::BasicBlock*> MetaCodegen::codegenMeta(llvm::BasicBlock* block,
                                                     ASTNode const* node) {

  // This function shall only expand contributed (useful and non intermediate)
  // nodes which can be passes to a normal function codegen.
  // If you are facing this assert you probably need to add an overload
  // for the asserting meta node.
  assert(!isNodeIntermediate(node) &&
         "Possibly tried to pass an IntermediateNode to a function codegen!");

  // Contribute the node to the layout
  return traverseNode(node, [&](auto promoted) {
    // Move the cursor downstairs
    cursor.descend(promoted->getKind());

    // Write the contribution of the node into the IR
    createContributeNode(node);

    /// Yield every node into a stack
    /// Post parse the stack to fork container nodes which contain subnodes
    // http://stackoverflow.com/questions/23888892/create-literal-pointer-value-in-llvm
    auto current = codegenChildrenContribution(block, node);

    if (ASTLayoutWriter::isNodeRequiringReduceMarker(node)) {
      // Insert a reduce marker when the node has ended
      createReduceNode(node);
    }

    // Move the cursor upstairs
    cursor.ascend(promoted->getKind());
    return current;
  });
}

void MetaCodegen::codegenJumpPad(llvm::Constant* callee,
                                 llvm::ArrayRef<ExprASTNode const*> args) {
  assert(function_->empty() && "Expected an empty function!");
  assert(function_->arg_size() == 1 &&
         "Expected the function to only have 1 arg!");

  FunctionCodegen::ValueMap::ScopeTy scope(functionCodegen_.values_);
  functionCodegen_.createEntryBlock();

  auto contextArg = &(*function_->arg_begin());

  llvm::SmallVector<llvm::Value*, 5> parameters{contextArg};

  for (auto arg : args) {
    parameters.push_back(functionCodegen_.codegenExpr(arg));
  }

  functionCodegen_.builder_.CreateCall(callee, parameters);
  functionCodegen_.builder_.CreateRetVoid();
}

Nullable<llvm::BasicBlock*>
MetaCodegen::codegenChildrenContribution(llvm::BasicBlock* block,
                                         ASTNode const* node) {
  auto current = makeNullable(block);
  traverseNodeIf(node, pred::hasChildren(), [&](auto promoted) {
    for (auto child : promoted->children()) {
      // When a node isn't known as meta node it's a meta contribution
      // which means we just continue and codegen all it's children.
      current = traverseNode(child, [&](auto* promotedChild) {
        auto result = codegenMeta(*current, promotedChild);
        return result;
      });
    }
  });
  return current;
}

void MetaCodegen::setMetaDataOf(llvm::Instruction* instr,
                                llvm::StringRef action, ASTNode const* node) {
  auto name = fmt::format("{} {}", action, ASTStringer::toTypeString(node));
  // functionCodegen_.builder_.CreateAlloca(getTypeOfInt(), nullptr, name);*/
  instr->setMetadata(
      100, llvm::MDNode::get(getLLVMContext(),
                             llvm::MDString::get(getLLVMContext(), name)));
}

llvm::Value* MetaCodegen::getContextArgument() const {
  assert(function_->arg_size() >= 1 &&
         "Expected the function to contain at least a context arg!");
  return &*function_->arg_begin();
}

void MetaCodegen::createContributeNode(ASTNode const* node) {
  // Get the prototype to the native callback
  auto callback = createContributeCallbackPrototype();

  // Get the callback arguments which are the pointer to the context and
  // a pointer to the node we want to contribute to the layout.
  auto context = getContextArgument();

  auto ptr = getPointerToNode(node);
  // TODO ptr->setName(ASTStringer::toTypeString(node));

  // Finally call the native callback which adds the node to the layout
  auto call = functionCodegen_.builder_.CreateCall(callback, {context, ptr});
  setMetaDataOf(call, "contributed", node);
}

void MetaCodegen::createReduceNode(ASTNode const* node) {
  // Get the prototype to the native callback
  auto callback = createReduceCallbackPrototype();

  // Get the callback argument which is the pointer to the context
  auto context = getContextArgument();

  // Finally call the native callback which reduces the layout
  auto call = functionCodegen_.builder_.CreateCall(callback, {context});
  setMetaDataOf(call, "reduced", node);
}

void MetaCodegen::createIntroduceNode(NamedDeclContext const* decl,
                                      llvm::Value* value) {
  auto callback = createIntroduceCallbackPrototype();
  auto context = getContextArgument();

  auto nodePtr = getPointerToNode(decl->getDeclaringNode());

  auto valuePtr =
      functionCodegen_.builder_.CreatePointerCast(value, getTypeOfContextPtr());

  auto level =
      llvm::ConstantInt::get(getTypeOfInt(), unsigned(cursor.getDepth()));

  // Finally call the introduce callback
  auto call = functionCodegen_.builder_.CreateCall(
      callback, {context, nodePtr, valuePtr, level});
  setMetaDataOf(call, "introduced", decl->getDeclaringNode());
}

llvm::Value* MetaCodegen::getPointerToNode(ASTNode const* node) {
  auto constant = getPointerConstant(node);
  return functionCodegen_.builder_.CreateIntToPtr(constant,
                                                  getTypeOfContextPtr());
}
