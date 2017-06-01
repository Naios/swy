


<p align="center">
	<img alt="Compiler" src="https://cloud.githubusercontent.com/assets/1146834/25398311/d0d4f4e4-29eb-11e7-967b-0b3855bac2fe.png" width="600">
</p>
<p align="center">
	<b>swy</b>
	<br>A static language for homogeneous meta-programming
</p>

---

## Introduction

This is my personal experimental educational research compiler which originally powered my bachelor's thesis: *"An Imperative Programming Language with Compile-time Homogeneous Meta-Programming"*. It's built on *LLVM* and *ANTLR* and features my own language for homogeneous meta-programming where we can do compile-time evaluations with the same language we use for run-time programming. Internally the compiler uses an LLVM ORC-JIT for evaluating expressions at compile-time as well as generating new code based upon those evaluations.

To be honest, I always wanted to invent my own programming language (like most enthusiastic developers), so I took the chance when I thought about a possible topic for a bachelor's thesis while keeping a special research in mind.

The overall project was developed in ~40 days, currently it's far away from being perfect, however I tried to keep the standards in software design high.

## Software Architecture

The compilation pipeline slightly differs from existing languages for allowing homogeneous meta-programming, the compiler uses a flat unstructured representation of AST nodes which makes deep cloning easy:

<p align="center">
  <img alt="Pipeline" src="https://cdn.rawgit.com/Naios/assets/88323ba3/diag.svg" width="500">
</p>

## Design patterns

The compiler itself uses a lot of C++ meta programming, for instance there are traversel functions which can up-cast AST nodes to their real type (this is a modern visitor replacement):

```c++
ASTNode* node = ...;

traverseNodeExpecting(node, pred::hasChildren(), [&](auto promoted) {
  // Static AST predicates: ^^^^^^^^^^^^^^^^^^^ we expect a node with children

  // `promoted` has the type of the node, the lambda is instantiated multiple times
  return std::count(promoted->children.begin(), promoted->children.end());
});
```


## Requirements

### Installation Instructions

#### Windows

Requirements:

- CMake
- Java 8

Build LLVM and run the CMake GUI on the compiler source

Select the LLVM cmake config

#### Linux

Requirements:

- CMake
- Java 8
   ​

Due to the usage of the ANTLR framework we have to build LLVM with RTTI support enabled:

```sh
git clone http://llvm.org/git/llvm.git
cd llvm
git checkout release_39

mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/llvm-rtti \
         -DCMAKE_BUILD_TYPE=Release \
         -DLLVM_ENABLE_ASSERTIONS=OFF \
         -DLLVM_TARGETS_TO_BUILD="X86" \
         -DLLVM_ENABLE_RTTI=ON

make -j8
make install
```

Build the compiler:

```sh
mkdir build
cd build

cmake -DLLVM_DIR=~/llvm-rtti/lib/cmake/llvm \
      -DANTLR_USE_PATH_JAVA=ON -DUSE_PCH=OFF ..

make -j8
```


