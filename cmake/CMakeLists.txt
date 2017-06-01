include("${CMAKE_CURRENT_LIST_DIR}/configure_macros.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/configure_platform.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/configure_compiler.cmake")

include(InstallRequiredSystemLibraries)
include(CPack)
