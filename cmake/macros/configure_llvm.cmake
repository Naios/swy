find_package(LLVM 3.9.1 EXACT REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

llvm_map_components_to_libnames(llvm_libs
  native
  target
  support
  core
  option
  irreader
  passes
  orcjit
  interpreter
  mc
  mcdisassembler
  mcjit
  mcparser
  objcarcopts
)

add_library(llvm INTERFACE IMPORTED GLOBAL)

set_property(TARGET llvm PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES
    "${LLVM_INCLUDE_DIRS}")

set_property(TARGET llvm PROPERTY
  INTERFACE_LINK_LIBRARIES
    "${llvm_libs}")

add_definitions(${LLVM_DEFINITIONS})

# TODO
#set_property(TARGET llvm PROPERTY
#    INTERFACE_COMPILE_DEFINITIONS
#      "${LLVM_DEFINITIONS}")

set_property(TARGET llvm PROPERTY
  INTERFACE_COMPILE_FEATURES
    cxx_defaulted_functions
    cxx_defaulted_move_initializers
    cxx_decltype_auto
    cxx_decltype
    cxx_constexpr
    cxx_auto_type)
