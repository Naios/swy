
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

#include "llvm/Support/CommandLine.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

#include "CompilerInstance.hpp"
#include "CompilerInvocation.hpp"
#include "Formatting.hpp"

using namespace llvm;

static cl::OptionCategory toolingOptionCat("Tooling Options");

static cl::opt<EmitAction> emitAction(
    cl::desc("Choose the emit action:"), cl::cat(toolingOptionCat),
    cl::values(
        clEnumValN(EmitAction::EmitTokens, "emit-tokens",
                   "Emits the tokens after lexing and exits"),
        clEnumValN(
            EmitAction::EmitFlatLayout, "emit-flat-layout",
            "Emits the flat layout (unstructured) after parsing and exits"),
        clEnumValN(EmitAction::EmitLayout, "emit-layout",
                   "Emits the layout after parsing and exits"),
        clEnumValN(EmitAction::EmitAST, "emit-ast",
                   "Emits the AST after layouting and exits"),
        clEnumValEnd));

static cl::OptionCategory optimizationOptionCat("Optimization Options");

static cl::opt<OptLevel> optLevel(
    cl::desc("Choose the optimization level for runtime code:"),
    cl::init(OptLevel::Debug), cl::cat(optimizationOptionCat),
    cl::values(
        clEnumValN(OptLevel::Debug, "O0", "Perform no optimizations (default)"),
        clEnumValN(OptLevel::O1, "O1", "Perform trivial optimizations"),
        clEnumValN(OptLevel::O2, "O2", "Perform default optimizations"),
        clEnumValN(OptLevel::O3, "O3", "Perform expensive optimizations"),
        clEnumValEnd));

static cl::OptionCategory debuggingOptionCat("Debugging Options");

static cl::bits<VerboseFlag> verboseFlags(
    cl::desc("Available verbose flags:"), cl::cat(debuggingOptionCat),
    cl::values(clEnumValN(VerboseFlag::All, "verbose",
                          "Prints all all verbose messages"),
               clEnumValN(VerboseFlag::Shipments, "vshipments",
                          "Prints all shipments to the code executor JIT"),
               clEnumValN(VerboseFlag::Instantiations, "vinst",
                          "Prints all meta decl instantiations"),
               clEnumValN(VerboseFlag::InstantiatedLayout, "vinst-layout",
                          "Prints the AST layout of performed instantiations"),
               clEnumValN(VerboseFlag::InstantiatedAST, "vinst-ast",
                          "Prints the parsed AST of performed instantiations"),
               clEnumValN(VerboseFlag::InstantiatedExports, "vinst-exports",
                          "Prints the exported values of instantiations"),
               clEnumValEnd));

static cl::opt<std::string>
    inputFilename(cl::Positional, cl::init(SOURCE_DIRECTORY "/lang/main.swy"),
                  cl::desc("<input file>"));

void testsmth();

int main(int argc, char const* argv[]) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram prettyStackTrace(argc, argv);

  if (llvm::sys::Process::FixupStandardFileDescriptors()) {
    return 1;
  }

  cl::SetVersionPrinter([] {
    outs() << "Compiler powered by LLVM {} on {} \n"_format(LLVM_VERSION_STRING,
                                                            LLVM_HOST_TRIPLE);
    outs().flush();
  });

  llvm::cl::ParseCommandLineOptions(argc, argv);

  // Set up the invocation
  CompilerInvocation invocation;
  invocation.setEmitAction(emitAction.getValue());
  invocation.setOptLevel(optLevel.getValue());
  invocation.setVerboseFlags(verboseFlags.getBits());

  // Start the compiler instance
  if (auto compiler = CompilerInstance::create(invocation)) {
    // testJit();
    auto module = compiler->compileSourceFile(inputFilename.getValue());
  }

  llvm::llvm_shutdown();
  return 0;
}
