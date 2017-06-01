
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

#include "FunctionCodegen.hpp"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include "AST.hpp"
#include "ASTPredicate.hpp"
#include "ASTTraversal.hpp"
#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"
#include "CompilerInvocation.hpp"
#include "DiagnosticEngine.hpp"
#include "Formatting.hpp"

FunctionCodegen::FunctionCodegen(IRContext* context, llvm::Function* function)
    : IRContextReplication(context), function_(function),
      builder_(context->getLLVMContext()) {}

llvm::Value* FunctionCodegen::lookupLocal(NamedDeclContext const* decl) {
  auto value = values_.lookup(decl);
  assert(value && "The value of the decl should be present!");
  return value;
}

void FunctionCodegen::codegen(FunctionDeclASTNode const* node) {
  ValueMap::ScopeTy scope(values_);
  auto block = setUpStackFrame(node->getArgDeclList());

  if (codegenStmt(block, node->getBody())) {
    // Insert a return void instr when we can still continue the scope
    builder_.CreateRetVoid();
  }
}

llvm::BasicBlock* FunctionCodegen::createEntryBlock() {
  assert(function_->empty() && "Expected an empty function");

  auto block = createBlock("entry");
  builder_.SetInsertPoint(block);
  return block;
}

llvm::BasicBlock* FunctionCodegen::createBlock(llvm::StringRef name) {
  return createBlockBefore(nullptr, name);
}

llvm::BasicBlock* FunctionCodegen::createBlockBefore(llvm::BasicBlock* block,
                                                     llvm::StringRef name) {
  return llvm::BasicBlock::Create(getLLVMContext(), name, getFunction(), block);
}

llvm::BasicBlock* FunctionCodegen::createRegionAfter(llvm::BasicBlock* block,
                                                     llvm::StringRef name) {
  auto created = createBlock(name);
  builder_.CreateBr(created);
  block->moveAfter(block);
  builder_.SetInsertPoint(created);
  return created;
}

llvm::BasicBlock*
FunctionCodegen::setUpStackFrame(ArgumentDeclListASTNode const* args,
                                 bool isReadOnly, bool isThisCall) {

  if (!isThisCall) {
    assert((args->children().size() == function_->arg_size()) &&
           "The given arguments must "
           "have the same count as "
           "the llvm::Function!");
  } else {
    assert((args->children().size() == (function_->arg_size() - 1)) &&
           "The given arguments must have the same count plus 1 (this "
           "argument) as the llvm::Function!");
  }

  auto block = createEntryBlock();

  auto itr = args->children().begin();
  auto range = getFunction()->args();

  if (isThisCall) {
    auto next = range.begin();
    ++next;
    range = {next, range.end()};
  }

  for (auto& arg : range) {
    if (auto named = llvm::dyn_cast<NamedArgumentDeclASTNode>(*itr)) {
      if (isReadOnly) {
        introduce(named, &arg);
      } else {
        std::string mangled = fmt::format("{}_framed", named->getName());
        auto allocaInst =
            builder_.CreateAlloca(arg.getType(), nullptr, mangled);
        builder_.CreateStore(&arg, allocaInst);
        introduce(named, allocaInst);
      }
    }
    ++itr;
  }

  return block;
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* block, StmtASTNode const* stmt) {
  return traverseNodeExpecting(
      stmt, pred::isStmtNode(),
      [=](auto promoted) -> Nullable<llvm::BasicBlock*> {
        return codegenStmt(block, promoted);
      });
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* block,
                             CompoundStmtASTNode const* stmt) {

  if (false) { // Insert extra scopes
    auto scopeBlock = createBlock("scope");
    builder_.CreateBr(scopeBlock);
    builder_.SetInsertPoint(scopeBlock);
    block = scopeBlock;
  }

  // Open a new scope
  ValueMap::ScopeTy scope(values_);

  auto current = makeNullable(block);
  for (auto child : stmt->children()) {
    // TODO assert(block && "Didn't catch early termination in sema!");
    current = codegenStmt(*current, child);
  }
  return current;
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* block,
                             UnscopedCompoundStmtASTNode const* stmt) {
  assert(block->getModule() == getModule());

  auto current = makeNullable(block);
  for (auto child : stmt->children()) {
    // TODO assert(block && "Didn't catch early termination in sema!");
    current = codegenStmt(*current, child);
    assert(!current || (block->getModule() == current->getModule()));
  }

  assert(block->getModule() == getModule());
  return current;
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* /*block*/,
                             ReturnStmtASTNode const* stmt) {
  if (auto expr = stmt->getExpression()) {
    auto value = codegenExpr(*expr);
    builder_.CreateRet(loadMemory(value));
  } else {
    builder_.CreateRetVoid();
  }
  return nullptr; // Return null so we can't use this branch anymore
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* block,
                             ExpressionStmtASTNode const* stmt) {
  (void)codegenExpr(stmt->getExpression());
  return block;
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* block,
                             DeclStmtASTNode const* stmt) {
  auto expr = codegenExpr(stmt->getExpression());
  auto allocaInst =
      builder_.CreateAlloca(getTypeOf(stmt), nullptr, *stmt->getName());
  builder_.CreateStore(loadMemory(expr), allocaInst);
  introduce(stmt, allocaInst);
  return block;
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* /*block*/,
                             MetaIfStmtASTNode const* /*stmt*/) {
  llvm_unreachable("Detected meta if within the runtime codegen!");
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* /*block*/,
                             MetaCalculationStmtASTNode const* /*stmt*/) {
  llvm_unreachable("Detected meta scope within the runtime codegen!");
}

Nullable<llvm::BasicBlock*>
FunctionCodegen::codegenStmt(llvm::BasicBlock* block,
                             IfStmtASTNode const* stmt) {
  auto codegenTrue = [=](llvm::BasicBlock* current) {
    return codegenStmt(current, stmt->getTrueBranch());
  };

  if (stmt->getFalseBranch()) {
    auto codegenFalse = [=](llvm::BasicBlock* current) {
      return codegenStmt(current, *stmt->getFalseBranch());
    };
    return codegenIfStructure(block, stmt->getExpression(), codegenTrue,
                              CodegenSupplier(codegenFalse));
  } else {
    return codegenIfStructure(block, stmt->getExpression(), codegenTrue);
  }
}

llvm::Value* FunctionCodegen::codegenExpr(ExprASTNode const* expr) {
  return traverseNodeExpecting(
      expr, pred::isExprNode(),
      [=](auto promoted) -> llvm::Value* { return codegenExpr(promoted); });
}

llvm::Value* FunctionCodegen::codegenExpr(DeclRefExprASTNode const* expr) {
  assert(expr->isResolved() && "Tried to codegen an unresolved decl ref!");

  if (expr->getDecl()->isFunctionDecl()) {
    // Lookup the function inside the IRContext
    auto declaring = expr->getDecl()->getDeclaringNode();
    return lookupGlobal(llvm::cast<FunctionDeclASTNode>(declaring));
  } else if (expr->getDecl()->isVarDecl()) {
    // Lookup the variable locally
    return lookupLocal(*expr->getDecl());
  } else if (expr->getDecl()->isGlobalConstant()) {
    // Codegen the global constant into the current context
    auto constant = llvm::cast<GlobalConstantDeclASTNode>(
        expr->getDecl()->getDeclaringNode());
    return codegenExpr(constant->getExpression());
  }
  llvm_unreachable("Unhandled decl!");
}

llvm::Value*
FunctionCodegen::codegenExpr(MetaInstantiationExprASTNode const* expr) {
  auto global = lookupGlobal(expr, true);
  assert(global && "Expected all instantiations to be resolved "
                   "before the generation of the function!");
  return *global;
}

llvm::Value*
FunctionCodegen::codegenExpr(IntegerLiteralExprASTNode const* expr) {
  return llvm::ConstantInt::get(getTypeOf(expr), *expr->getLiteral());
}

llvm::Value*
FunctionCodegen::codegenExpr(BooleanLiteralExprASTNode const* expr) {
  return llvm::ConstantInt::get(getTypeOf(expr),
                                std::int32_t(*expr->getLiteral()));
}

llvm::Value*
FunctionCodegen::codegenExpr(ErroneousExprASTNode const* /*expr*/) {
  llvm_unreachable("Yeah nice, how were you able to pass the AST until here^^");
}

llvm::Value*
FunctionCodegen::codegenExpr(BinaryOperatorExprASTNode const* expr) {
  auto left = codegenExpr(expr->getLeftExpr());
  auto right = codegenExpr(expr->getRightExpr());

  // Because the llvm emitted type is i1 we have to cast it to i32 afterwards
  auto const buildComparisonOp = [&](auto&& builder) {
    auto leftLoaded = loadMemory(left);
    auto rightLoaded = loadMemory(right);
    auto intermediate = builder(leftLoaded, rightLoaded);
    return builder_.CreateCast(llvm::Instruction::CastOps::ZExt, intermediate,
                               leftLoaded->getType());
  };

  switch (*expr->getBinaryOperator()) {
    case ExprBinaryOperator::OperatorMul:
      return builder_.CreateMul(loadMemory(left), loadMemory(right));
    case ExprBinaryOperator::OperatorDiv:
      return builder_.CreateSDiv(loadMemory(left), loadMemory(right));
    case ExprBinaryOperator::OperatorPlus:
      return builder_.CreateAdd(loadMemory(left), loadMemory(right));
    case ExprBinaryOperator::OperatorMinus:
      return builder_.CreateSub(loadMemory(left), loadMemory(right));
    case ExprBinaryOperator::OperatorAssign:
      builder_.CreateStore(loadMemory(right), left);
      return right;
    case ExprBinaryOperator::OperatorLessThan:
      return buildComparisonOp([this](llvm::Value* left, llvm::Value* right) {
        return builder_.CreateICmpSLT(left, right);
      });
    case ExprBinaryOperator::OperatorGreaterThan:
      return buildComparisonOp([this](llvm::Value* left, llvm::Value* right) {
        return builder_.CreateICmpSGT(left, right);
      });
    case ExprBinaryOperator::OperatorLessThanOrEq:
      return buildComparisonOp([this](llvm::Value* left, llvm::Value* right) {
        return builder_.CreateICmpSLE(left, right);
      });
    case ExprBinaryOperator::OperatorGreaterThanOrEq:
      return buildComparisonOp([this](llvm::Value* left, llvm::Value* right) {
        return builder_.CreateICmpSGE(left, right);
      });
    case ExprBinaryOperator::OperatorEqual:
      return buildComparisonOp([this](llvm::Value* left, llvm::Value* right) {
        return builder_.CreateICmpEQ(left, right);
      });
    case ExprBinaryOperator::OperatorNotEqual:
      return buildComparisonOp([this](llvm::Value* left, llvm::Value* right) {
        return builder_.CreateICmpNE(left, right);
      });
    default:
      llvm_unreachable("Unhandled binary operator!");
  }
}

llvm::Value* FunctionCodegen::codegenExpr(CallOperatorExprASTNode const* expr) {
  auto callee = codegenExpr(expr->getCallee());

  // Create the parameter values
  llvm::SmallVector<llvm::Value*, 5> args;
  for (auto arg : expr->getExpressions()) {
    args.push_back(loadMemory(codegenExpr(arg)));
  }

  auto inst = builder_.CreateCall(callee, args);
  inst->setTailCall();
  return inst;
}

Nullable<llvm::BasicBlock*> FunctionCodegen::codegenIfStructure(
    llvm::BasicBlock* block, ExprASTNode const* condition,
    CodegenSupplier codegenTrue, llvm::Optional<CodegenSupplier> codegenFalse) {

  // Never create the continue block when both statements terminate
  // the control flow, so we create it lazily.
  Nullable<llvm::BasicBlock*> continueBlock;
  auto lazyGetContinueBlock = [&] {
    if (!continueBlock) {
      continueBlock = createBlock("continue_block");
    }
    return *continueBlock;
  };

  // Create the true branch
  auto trueBlock = createBlock("true_block");
  builder_.SetInsertPoint(trueBlock);

  if (codegenTrue(trueBlock)) {
    builder_.CreateBr(lazyGetContinueBlock());
  }

  // Create the false branch if any was specified
  auto falseBlock = [&] {
    if (codegenFalse) {
      auto falseStmtBlock = [&] {
        // Insert the block before the continue block if any exists
        auto const constexpr name = "false_block";
        if (continueBlock)
          return createBlockBefore(*continueBlock, name);
        else
          return createBlock(name);
      }();
      builder_.SetInsertPoint(falseStmtBlock);
      if ((*codegenFalse)(falseStmtBlock)) {
        builder_.CreateBr(lazyGetContinueBlock());
      }
      return falseStmtBlock;
    } else {
      return lazyGetContinueBlock();
    }
  }();

  // Create the branch instruction
  builder_.SetInsertPoint(block);
  auto conditionResult = codegenExpr(condition);
  auto predicate = builder_.CreateIsNotNull(conditionResult, "condition_test");
  builder_.CreateCondBr(predicate, trueBlock, falseBlock);

  if (continueBlock) {
    // Finally update the current block to the continue one
    builder_.SetInsertPoint(*continueBlock);
    return *continueBlock;
  } else {
    // Or when no one exists return the statement as terminated
    return nullptr;
  }
}

llvm::Value* FunctionCodegen::loadMemory(llvm::Value* value) {
  if (value->getType()->isPointerTy()) {
    return builder_.CreateLoad(value);
  }
  return value;
}
