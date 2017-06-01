# Select the platform specific cmake file
if (WIN32)
  include("${CMAKE_CURRENT_LIST_DIR}/platform/windows.cmake")
elseif (UNIX)
  include("${CMAKE_CURRENT_LIST_DIR}/platform/unix.cmake")
else()
  message(FATAL_ERROR "Unknown platform!")
endif()
