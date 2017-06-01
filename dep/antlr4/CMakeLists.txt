set(CPP_RUNTIME_PATH "${CMAKE_CURRENT_LIST_DIR}/antlr4/runtime/Cpp/runtime/src")

file(GLOB_RECURSE ANTLR_SOURCES
  ${CPP_RUNTIME_PATH}/*.h
  ${CPP_RUNTIME_PATH}/*.c
  ${CPP_RUNTIME_PATH}/*.hpp
  ${CPP_RUNTIME_PATH}/*.cpp
  ${CPP_RUNTIME_PATH}/*.cc
  ${CPP_RUNTIME_PATH}/*.hh)

add_library(antlr4 STATIC
  ${ANTLR_SOURCES})

target_include_directories(antlr4
  PUBLIC
    "${CPP_RUNTIME_PATH}"
  PRIVATE
    "${CPP_RUNTIME_PATH}/tree")

target_compile_definitions(antlr4
  PUBLIC
    "-DANTLR4CPP_STATIC")

target_compile_features(antlr4
  PUBLIC
    ${CXX14_FEATURES})

set_target_properties(antlr4
 PROPERTIES
   FOLDER
     "dep")

file(DOWNLOAD
  http://www.antlr.org/download/antlr-4.7-complete.jar
  ${CMAKE_BINARY_DIR}/antlr4-complete.jar
  SHOW_PROGRESS
  EXPECTED_HASH SHA512=4172eb0d72a57aaff913cafe9e1ca36d29eec7241a9c495c5bef6009e4938c6aac887c93e273c5b088e97420746a13dfc12e7fac0c9776745b101a5da66c3888)

if (ANTLR_USE_PATH_JAVA)
  set(Java_JAVA_EXECUTABLE "java" CACHE STRING "Java")
else()
  find_package(Java REQUIRED)
endif()
