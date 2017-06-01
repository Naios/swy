
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

#ifndef CODEGEN_BASE_HPP_INCLUDED__
#define CODEGEN_BASE_HPP_INCLUDED__

#include <string>

#include "llvm/ADT/StringRef.h"

namespace llvm {
class LLVMContext;
class Type;
class FunctionType;
class Function;
class Module;
class Constant;
}

class CompilationUnit;
class UnitASTNode;
class FunctionDeclASTNode;
class MetaDeclASTNode;
class IntegerLiteralExprASTNode;
class BooleanLiteralExprASTNode;
class DeclStmtASTNode;
class AnonymousArgumentDeclASTNode;
class MetaInstantiationExprASTNode;

/// Provides basic support methods for codegen classes,
/// which usually are a bridge between ASTNodes and it's llvm::Type's.
template <typename Base> class CodegenBase {
public:
  CodegenBase() = default;

  /// Returns the type of an integer literal
  llvm::Type* getTypeOf(IntegerLiteralExprASTNode const* node);
  /// Returns the type of a boolean literal
  llvm::Type* getTypeOf(BooleanLiteralExprASTNode const* node);
  /// Returns the type of a DeclStmtASTNode
  llvm::Type* getTypeOf(DeclStmtASTNode const* node);
  /// Returns the llvm type of the given AnonymousArgumentDeclASTNode
  llvm::Type* getTypeOf(AnonymousArgumentDeclASTNode const* node);

  /// Returns the type of int
  llvm::Type* getTypeOfInt();
  /// Returns the type of void
  llvm::Type* getTypeOfVoid();

  /// Returns the function type of the given FunctionDeclASTNode
  llvm::FunctionType* getFunctionTypeOf(FunctionDeclASTNode const* node);
  /// Returns the meta function type of the given MetaDeclASTNode
  /// which is used to generate the yield of a meta instantiation.
  llvm::FunctionType* getFunctionTypeOf(MetaDeclASTNode const* node);

  /// Returns the mangled name of the given global node
  static std::string mangleSymbolOf(llvm::StringRef name);
  /// Returns the mangled name of the given global node
  static std::string mangleNameOf(FunctionDeclASTNode const* node);
  /// Returns the mangled name of the given global node
  static std::string mangleNameOf(MetaDeclASTNode const* node);
  /// Returns the mangled name of the given global node
  static std::string mangleNameOf(MetaInstantiationExprASTNode const* node);

  /// Returns a forward declaration to the given function
  llvm::Constant* createFunctionPrototype(llvm::Function* function);
  /// Returns a forward declaration to the given function
  llvm::Constant* createFunctionPrototype(FunctionDeclASTNode const* node);
  /// Returns a forward declaration to the given function
  llvm::Constant* createFunctionPrototype(MetaDeclASTNode const* node);

  /// Creates a function with the given name and type into the current module
  llvm::Function* createFunction(llvm::StringRef name,
                                 llvm::FunctionType* type);

  /// Creates a function with the given name and type into the given module
  static llvm::Function* createFunction(llvm::Module* module,
                                        llvm::StringRef name,
                                        llvm::FunctionType* type);

  /// Returns the type of the (first) context argument in meta functions
  /// which usually references to the ASTLayoutWriter object.
  llvm::Type* getTypeOfContextPtr();
  /// Returns a prototype of the contribution callback
  llvm::Constant* createContributeCallbackPrototype();
  /// Returns a prototype of the reduce callback
  llvm::Constant* createReduceCallbackPrototype();
  /// Returns a prototype of the introduce callback
  llvm::Constant* createIntroduceCallbackPrototype();

  /// Returns a constant to the given pointer
  llvm::Constant* getPointerConstant(void const* ptr);

  /// Returns true when no error was submitted to the
  /// associated DiagnosticEngine
  bool canContinue();

private:
  llvm::LLVMContext& context();
  llvm::Module* module();
  CompilationUnit* compilationUnit();
};

#endif // #ifndef CODEGEN_BASE_HPP_INCLUDED__
