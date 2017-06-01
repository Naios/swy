# Parser & Lexer
list(APPEND GENERATED_SOURCES
  "${CMAKE_BINARY_DIR}/Generated/GeneratedLexer.h"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedLexer.cpp"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParser.h"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParser.cpp"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParserBaseListener.cpp"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParserBaseListener.h"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParserBaseVisitor.cpp"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParserBaseVisitor.h"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParserListener.cpp"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParserListener.h"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParserVisitor.cpp"
  "${CMAKE_BINARY_DIR}/Generated/GeneratedParserVisitor.h")

add_custom_command(
  OUTPUT
    ${GENERATED_SOURCES}
  COMMAND
    ${Java_JAVA_EXECUTABLE}
      -jar ${CMAKE_BINARY_DIR}/antlr4-complete.jar
        -Dlanguage=Cpp -listener -visitor -o ${CMAKE_BINARY_DIR}/Generated
        ${CMAKE_CURRENT_LIST_DIR}/Parse/GeneratedLexer.g4
        ${CMAKE_CURRENT_LIST_DIR}/Parse/GeneratedParser.g4
  DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/Parse/GeneratedLexer.g4"
    "${CMAKE_CURRENT_LIST_DIR}/Parse/GeneratedParser.g4"
  VERBATIM)

add_custom_target(GenerateLexerParser
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/Parse/GeneratedLexer.g4"
    "${CMAKE_CURRENT_LIST_DIR}/Parse/GeneratedParser.g4"
  DEPENDS
    ${GENERATED_SOURCES})

source_group("Parse\\Generated"
  FILES
    ${GENERATED_SOURCES})

# Compiler
CollectSourceFiles(
  ${CMAKE_CURRENT_LIST_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_LIST_DIR}/PCH)

CollectIncludeDirectories(
  ${CMAKE_CURRENT_LIST_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_LIST_DIR}/PCH)

if (USE_PCH)
  set(PRIVATE_PCH_HEADER ${CMAKE_CURRENT_LIST_DIR}/PCH/PCH.hpp)
endif()

add_executable(compiler
  ${PRIVATE_SOURCES}
  ${GENERATED_SOURCES}
  ${CMAKE_SOURCE_DIR}/lang/main.swy)

add_dependencies(compiler GenerateLexerParser)

target_link_libraries(compiler
  PUBLIC
    fmt
    llvm
  PRIVATE
    antlr4
    function2)

target_include_directories(compiler
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/AST"
    "${CMAKE_CURRENT_LIST_DIR}/Codegen"
    "${CMAKE_CURRENT_LIST_DIR}/Diag"
    "${CMAKE_CURRENT_LIST_DIR}/Frontend"
    "${CMAKE_CURRENT_LIST_DIR}/Parse"
    "${CMAKE_CURRENT_LIST_DIR}/Sema"
    "${CMAKE_CURRENT_LIST_DIR}/Support"
    "${CMAKE_CURRENT_LIST_DIR}/Tooling"
    "${CMAKE_BINARY_DIR}/Generated")

target_compile_features(compiler
  PRIVATE
    ${CXX14_FEATURES}
  PUBLIC
    ${CXX11_FEATURES})

if(USE_PCH)
  set_target_properties(compiler
    PROPERTIES
      # Set the PCH header
      COTIRE_CXX_PREFIX_HEADER_INIT "${PRIVATE_PCH_HEADER}"
      # Disable unity builds
      COTIRE_ADD_UNITY_BUILD FALSE)

  cotire(compiler)
endif()

target_compile_definitions(compiler
  PRIVATE
    "-DSOURCE_DIRECTORY=\"${CMAKE_SOURCE_DIR}\"")

group_sources(${CMAKE_CURRENT_LIST_DIR})
# source_group("\\Parse\\Generated" FILES ${GENERATED_SOURCES})
