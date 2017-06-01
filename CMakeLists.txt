cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(compiler LANGUAGES C CXX)

enable_testing()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(USE_PCH "Use precompiled headers" ON)

include(cmake/CMakeLists.txt)
if (USE_PCH)
  include(dep/cotire/CMakeLists.txt)
endif()

add_subdirectory(dep)

add_subdirectory(src)
