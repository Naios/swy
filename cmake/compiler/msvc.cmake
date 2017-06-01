if (${MSVC_VERSION} LESS 1900)
  message(FATAL_ERROR "You are using an unsupported version of Visual Studio "
                      "which doesn't support all required C++14 features. "
                      "(Visual Studio 2015 (version >= 1900) is required!)")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(PLATFORM 64)
else()
  set(PLATFORM 32)
endif()

if (PLATFORM EQUAL 64)
  add_definitions("-D_WIN64")
endif()

# Enable full warnings
string(REGEX REPLACE "/W3" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

# Enable multithreaded compilation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

# Disable exceptions
# add_definitions(-D_HAS_EXCEPTIONS=0)
# string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
