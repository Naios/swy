
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

#ifndef CODE_EXECUTOR_HPP_INCLUDED__
#define CODE_EXECUTOR_HPP_INCLUDED__

#include <unordered_map>
#include <unordered_set>

#include "llvm/ADT/PointerUnion.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "CodegenBase.hpp"
#include "Hash.hpp"
#include "IRContext.hpp"
#include "Nullable.hpp"

namespace llvm {
class Function;
class Module;
}

class ASTContext;
class MetaUnitASTNode;
class MetaDeclASTNode;
class MetaInstantiationExprASTNode;
class FunctionDeclASTNode;

/// An execution engine to evaluate compile-time computations
/// and meta functions.
/// The CodeExecutor provides methods for pulling differential sets
/// of functions from the amalgamation into the executor.
class CodeExecutor : public IRContext, public CodegenBase<CodeExecutor> {
  IRContext* context_;
  std::unique_ptr<llvm::ExecutionEngine> executor_;
  /// Contains all symbols which are already available
  /// and usable in the ExecutionEngine.
  std::unordered_set<std::string> availableSymbols_;
  /// Contains all unresolved entities which need to be resolved
  /// before the shipment can be successfully transferred to the JIT.
  std::unordered_set<
      llvm::PointerUnion3<llvm::Function*, FunctionDeclASTNode const*,
                          MetaInstantiationExprASTNode const*>,
      PointerUnionHasher, PointerUnionEquality>
      unresolvedDependencies_;
  /// Additions to the JIT are kept lazily inside a module and submitted
  /// transactional upon usage. Use the shipment() method for lazy retrieval.
  std::unique_ptr<llvm::Module> shipment_;
  /// Caches the result of meta instantiation
  std::unordered_map<MetaInstantiationExprASTNode const*,
                     MetaUnitASTNode const*>
      instantiations_;

  explicit CodeExecutor(IRContext* context) : context_(context) {}

  void setExecutor(std::unique_ptr<llvm::ExecutionEngine> executor);

public:
  /// Tries to create a CodeExecutor for the given IRContext
  /// TODO Replace Optional by Expected
  static llvm::Optional<CodeExecutor> createFor(IRContext* context);

  CompilationUnit* getCompilationUnit() override;
  ASTContext* getASTContext() override;
  llvm::LLVMContext& getLLVMContext() override;
  llvm::Module* getModule() override;

  llvm::Constant* lookupGlobal(FunctionDeclASTNode const* function) override;
  Nullable<llvm::Constant*>
  lookupGlobal(MetaInstantiationExprASTNode const* inst,
               bool requiresCompleted = false) override;
  Nullable<llvm::Constant*>
  resolveGlobal(llvm::Function const* function) override;
  Nullable<llvm::Constant*>
  resolveGlobal(FunctionDeclASTNode const* function) override;
  Nullable<llvm::Constant*>
  resolveGlobal(MetaInstantiationExprASTNode const* inst) override;

  /// Instantiates a MetaInstantiationExprASTNode
  Nullable<MetaUnitASTNode const*>
  instantiate(MetaInstantiationExprASTNode const* inst,
              bool requiresCompleted = false);

private:
  /// Initializes and returns the shipment lazily.
  llvm::Module* shipment();
  /// Do additional initializations to the module for the executor
  void initializeModule(llvm::Module* module);
  /// Transfers the current shipment to the JIT while resolving
  /// all unresolved dependencies first.
  /// Returns true when the shipping was succesfull
  bool shipToJIT();

  /// Returns true when the given global name is available in the executor
  /// already or when the next shipment was transferred to the JIT.
  bool isGlobalResolved(llvm::StringRef symbol) const;
  /// Sets the global as available
  void setGlobalAsResolved(std::string symbol);
  /// Sets the given global as unresolved which will cause the executor
  /// to pull it lazily into the JIT before the next shipment.
  void
  setGlobalAsUnresolved(decltype(unresolvedDependencies_)::value_type entity);
  /// Resolves all unresolved dependencies and returns true when it was
  /// successfull
  bool resolveDependencies();

  /// Codegens the meta function for the given MetaDeclASTNode
  /// Returns an empty error when an instantiation error occured
  Nullable<llvm::Function*> codegen(MetaDeclASTNode const* node);

  /// Pulls the function and all it's dependencies into the executor.
  llvm::Function* pull(llvm::Function* function);

  /// Clones the function prototype into the current shipment
  llvm::Constant* cloneFunctionPrototype(llvm::Function* function);

  /// Returns a cached instantiation if there is any
  Nullable<MetaUnitASTNode const*>
  getCachedInstantiationOf(MetaInstantiationExprASTNode const* inst);
  /// Cache the instantiation of the given MetaInstantiationExprASTNode
  void cacheInstantiationOf(MetaInstantiationExprASTNode const* inst,
                            MetaUnitASTNode const* unit);

  /// Creates the jump pad which is used to enter the given meta instantiation.
  /// Basically this lowers a meta instantiation to be called with a
  /// `void(void*)` signature since the arguments of a meta instantiation
  /// theoretically are allowed to contain intermediate expressions as well.
  llvm::Function* createJumpPadTo(MetaInstantiationExprASTNode const* inst,
                                  llvm::Constant* metafunction);

  /// Internal meta compile callback to contribute a node to the given context.
  static void contributeNodeCallback(void* context, void* node);
  /// Internal meta compile callback to reduce a certain node.
  static void reduceNodeCallback(void* context);
  /// Internal meta compile callback to introduce new nodes.
  static void introduceNodeCallback(void* context, void* node, void* value,
                                    unsigned depth);
};

#endif // #ifndef CODE_EXECUTOR_HPP_INCLUDED__
