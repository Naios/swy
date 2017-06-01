
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

#include "CodegenBase.hpp"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

#include "AST.hpp"
#include "CodeExecutor.hpp"
#include "CodegenInstance.hpp"
#include "CompilationUnit.hpp"
#include "CompilerInstance.hpp"
#include "CompilerInvocation.hpp"
#include "Formatting.hpp"
#include "FunctionCodegen.hpp"
#include "MetaCodegen.hpp"
#include "NameMangeling.hpp"

template <typename Base>
static llvm::FunctionType*
getFunctionType(CodegenBase<Base>* base,
                ArgumentDeclListASTNode const* argDeclList,
                Nullable<AnonymousArgumentDeclASTNode const*> returnType,
                bool isThisCall = false) {
  // Get the argument types
  llvm::SmallVector<llvm::Type*, 10> args;

  if (isThisCall) {
    args.push_back(base->getTypeOfContextPtr());
  }

  for (auto arg : argDeclList->children()) {
    args.push_back(base->getTypeOf(arg));
  }

  // If the return type is not specified it's void
  llvm::Type* result = [&] {
    if (returnType)
      return base->getTypeOf(*returnType);
    else
      return base->getTypeOfVoid();
  }();

  return llvm::FunctionType::get(result, args, false);
}

template <typename Base>
llvm::Type*
CodegenBase<Base>::getTypeOf(IntegerLiteralExprASTNode const* /*node*/) {
  return getTypeOfInt();
}

template <typename Base>
llvm::Type*
CodegenBase<Base>::getTypeOf(BooleanLiteralExprASTNode const* /*node*/) {
  return getTypeOfInt();
}

template <typename Base>
llvm::Type* CodegenBase<Base>::getTypeOf(DeclStmtASTNode const* /*node*/) {
  return getTypeOfInt();
}

template <typename Base>
llvm::Type*
CodegenBase<Base>::getTypeOf(AnonymousArgumentDeclASTNode const* /*node*/) {
  return getTypeOfInt();
}

template <typename Base> llvm::Type* CodegenBase<Base>::getTypeOfInt() {
  return llvm::Type::getInt32Ty(context());
}

template <typename Base> llvm::Type* CodegenBase<Base>::getTypeOfVoid() {
  return llvm::Type::getVoidTy(context());
}

template <typename Base>
llvm::FunctionType*
CodegenBase<Base>::getFunctionTypeOf(FunctionDeclASTNode const* node) {
  return getFunctionType(this, node->getArgDeclList(), node->getReturnType());
}

template <typename Base>
llvm::FunctionType*
CodegenBase<Base>::getFunctionTypeOf(MetaDeclASTNode const* node) {
  return getFunctionType(this, node->getArgDeclList(), nullptr, true);
}

template <typename T> std::string setupStream(T&& mangeler) {
  llvm::SmallString<50> buffer;
  llvm::raw_svector_ostream stream(buffer);
  std::forward<T>(mangeler)(stream);
  return stream.str();
}

template <typename Base>
std::string CodegenBase<Base>::mangleSymbolOf(llvm::StringRef name) {
  return setupStream(
      [=](auto& stream) { mangeling::mangleSymbol(stream, name); });
}

template <typename Base>
std::string CodegenBase<Base>::mangleNameOf(FunctionDeclASTNode const* node) {
  return setupStream(
      [=](auto& stream) { mangeling::mangleNameOf(stream, node); });
}

template <typename Base>
std::string CodegenBase<Base>::mangleNameOf(MetaDeclASTNode const* node) {
  return setupStream(
      [=](auto& stream) { mangeling::mangleNameOf(stream, node); });
}

template <typename Base>
std::string
CodegenBase<Base>::mangleNameOf(MetaInstantiationExprASTNode const* node) {
  return setupStream(
      [=](auto& stream) { mangeling::mangleNameOf(stream, node); });
}

template <typename Base>
llvm::Constant*
CodegenBase<Base>::createFunctionPrototype(llvm::Function* function) {
  return module()->getOrInsertFunction(function->getName(),
                                       function->getFunctionType());
}

template <typename Base>
llvm::Constant*
CodegenBase<Base>::createFunctionPrototype(FunctionDeclASTNode const* node) {
  return module()->getOrInsertFunction(mangleNameOf(node),
                                       getFunctionTypeOf(node));
}

template <typename Base>
llvm::Constant*
CodegenBase<Base>::createFunctionPrototype(MetaDeclASTNode const* node) {
  return module()->getOrInsertFunction(mangleNameOf(node),
                                       getFunctionTypeOf(node));
}

template <typename Base>
llvm::Function* CodegenBase<Base>::createFunction(llvm::StringRef name,
                                                  llvm::FunctionType* type) {
  return createFunction(module(), name, type);
}

template <typename Base>
llvm::Function* CodegenBase<Base>::createFunction(llvm::Module* module,
                                                  llvm::StringRef name,
                                                  llvm::FunctionType* type) {
  return llvm::Function::Create(type, llvm::GlobalValue::ExternalLinkage, name,
                                module);
}

template <typename Base> llvm::Type* CodegenBase<Base>::getTypeOfContextPtr() {
  auto structure = llvm::StructType::get(context(), {});
  return llvm::PointerType::getUnqual(structure);
}

template <typename Base>
llvm::Constant* CodegenBase<Base>::createContributeCallbackPrototype() {
  // The internal name of the contribute callback
  static auto name = mangleSymbolOf("callback_contribute");

  // The function type which is used to contribute nodes to the current context.
  auto type = llvm::FunctionType::get(
      getTypeOfVoid(), {getTypeOfContextPtr(), getTypeOfContextPtr()}, false);

  return module()->getOrInsertFunction(name, type);
}

template <typename Base>
llvm::Constant* CodegenBase<Base>::createReduceCallbackPrototype() {
  // The internal name of the reduce callback
  static auto name = mangleSymbolOf("callback_reduce");

  // The function type which is used to reduce the current context.
  auto type =
      llvm::FunctionType::get(getTypeOfVoid(), {getTypeOfContextPtr()}, false);

  return module()->getOrInsertFunction(name, type);
}

template <typename Base>
llvm::Constant* CodegenBase<Base>::createIntroduceCallbackPrototype() {
  // The internal name of the introduce callback
  static auto name = mangleSymbolOf("callback_introduce");

  // The function type which is used to introduce node into the current context.
  auto type = llvm::FunctionType::get(
      getTypeOfVoid(), {getTypeOfContextPtr(), getTypeOfContextPtr(),
                        getTypeOfContextPtr(), getTypeOfInt()},
      false);

  return module()->getOrInsertFunction(name, type);
}

template <typename Base>
llvm::Constant* CodegenBase<Base>::getPointerConstant(void const* ptr) {
  // TODO make the pointer size depend on the host platform
  auto type = llvm::Type::getInt64Ty(context());
  return llvm::ConstantInt::get(type, reinterpret_cast<std::uint64_t>(ptr));
}

template <typename Base> bool CodegenBase<Base>::canContinue() {
  return !compilationUnit()->getDiagnosticEngine()->hasErrors();
}

template <typename Base> llvm::LLVMContext& CodegenBase<Base>::context() {
  return static_cast<Base*>(this)->getLLVMContext();
}

template <typename Base> llvm::Module* CodegenBase<Base>::module() {
  return static_cast<Base*>(this)->getModule();
}

template <typename Base> CompilationUnit* CodegenBase<Base>::compilationUnit() {
  return static_cast<Base*>(this)->getCompilationUnit();
}

template class CodegenBase<CodegenInstance>;
template class CodegenBase<FunctionCodegen>;
template class CodegenBase<MetaCodegen>;
template class CodegenBase<CodeExecutor>;
