include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(open io.h HAVE_OPEN)
else ()
  check_symbol_exists(open fcntl.h HAVE_OPEN)
endif ()

set(FMT_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/fmt/fmt/format.cc
  ${CMAKE_CURRENT_LIST_DIR}/fmt/fmt/ostream.cc)

if (HAVE_OPEN)
  set(FMT_SOURCES ${FMT_SOURCES}
      ${CMAKE_CURRENT_LIST_DIR}/fmt/fmt/posix.cc)
endif()

add_library(fmt STATIC ${FMT_SOURCES})

target_include_directories(fmt
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/fmt)

target_compile_definitions(fmt
  PUBLIC
    -DFMT_USE_VARIADIC_TEMPLATES=1
    -DFMT_USE_RVALUE_REFERENCES=1
    -DFMT_USE_DELETED_FUNCTIONS=1
    -DFMT_USE_USER_DEFINED_LITERALS=1
    -DFMT_EXCEPTIONS=0)

target_compile_features(fmt
  PUBLIC
    cxx_variadic_templates
    cxx_rvalue_references
    cxx_deleted_functions
    cxx_constexpr
    cxx_user_literals)
    
set_target_properties(fmt
 PROPERTIES
   FOLDER
     "dep")
