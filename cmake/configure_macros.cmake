# Register all macros
file(GLOB LOCALE_MACRO_FILES
  "${CMAKE_CURRENT_LIST_DIR}/macros/*.cmake"
)
foreach (LOCALE_MACRO_FILE ${LOCALE_MACRO_FILES})
  include("${LOCALE_MACRO_FILE}")
endforeach()
