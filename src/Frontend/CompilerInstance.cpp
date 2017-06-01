
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

#include "CompilerInstance.hpp"

#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

#include "CompilationUnit.hpp"
#include "CompilerInvocation.hpp"
#include "Nullable.hpp"

/// Creates a target machine from the given targte triple
static Nullable<llvm::TargetMachine*>
createTargetMachine(llvm::StringRef triple, bool isNative = false) {
  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(triple, error);
  if (!target) {
    llvm::errs() << "Failed to initialize target: '" << triple << "' (" << error
                 << ")!\n";
    return {};
  }

  auto cpu = [=]() -> llvm::StringRef {
    if (isNative) {
      return llvm::sys::getHostCPUName();
    } else {
      return "generic";
    }
  }();

  auto features = "";

  llvm::TargetOptions opt;
  llvm::Optional<llvm::Reloc::Model> rm;

  return target->createTargetMachine(triple, cpu, features, opt, rm);
}

std::unique_ptr<CompilerInstance>
CompilerInstance::create(CompilerInvocation const& compilerInvocation) {

  // Initialize all targets
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  // Initialize the target machines
  auto targetMachine =
      createTargetMachine(compilerInvocation.getTargetTriple());
  auto hostMachine =
      createTargetMachine(compilerInvocation.getTargetTriple(), true);
  if (!targetMachine || !hostMachine) {
    return {};
  }

  std::unique_ptr<CompilerInstance> instance(
      new CompilerInstance(compilerInvocation, *targetMachine, *hostMachine));

  // Initialize the PassRegistry which are basically
  // the same passes opt is doing

  // Pass list was taken from llvm\tools\opt\opt.cpp
  auto& registry = *llvm::PassRegistry::getPassRegistry();
  initializeCore(registry);
  initializeScalarOpts(registry);
  initializeObjCARCOpts(registry);
  initializeVectorization(registry);
  initializeIPO(registry);
  initializeAnalysis(registry);
  initializeTransformUtils(registry);
  initializeInstCombine(registry);
  initializeInstrumentation(registry);
  initializeTarget(registry);
  initializeCodeGenPreparePass(registry);
  initializeAtomicExpandPass(registry);
  initializeRewriteSymbolsPass(registry);
  initializeWinEHPreparePass(registry);
  initializeDwarfEHPreparePass(registry);
  initializeSafeStackPass(registry);
  initializeSjLjEHPreparePass(registry);
  initializePreISelIntrinsicLoweringLegacyPassPass(registry);
  initializeGlobalMergePass(registry);
  initializeInterleavedAccessPass(registry);
  initializeUnreachableBlockElimLegacyPassPass(registry);

  return instance;
}

llvm::ErrorOr<bool /*std::unique_ptr<llvm::Module>*/>
CompilerInstance::compileSourceFile(std::string path) {
  auto source = llvm::MemoryBuffer::getFile(path);
  if (!source) {
    logError("Didn't find file {}!", path);
    return llvm::ErrorOr<bool>(source);
  }

  unsigned id = sourceMgr.AddNewSourceBuffer(std::move(*source), {});

  CompilationUnit compilationUnit(this, id, path);

  compilationUnit.translate();

  return {false};
}

static llvm::StringRef const severities[] = {"INFO   ", "WARNING", "ERROR  "};

void CompilerInstance::logSeverity(Severity /*severity*/, llvm::StringRef msg) {
  llvm::outs() /*<< '[' << severities[unsigned(severity)] << "] "*/ << msg
                                                                    << "\n";
  llvm::outs().flush();
}
