# Project64 portable build: CMake with GCC or Clang (Linux first, MinGW next).
#
# This build only adds files: the MSVC solution (Project64.sln) and its
# projects stay as they are, and the source lists are read from the upstream
# .vcxproj files, so files added upstream are picked up without edits here.
#
#   cmake -S . -B <build> [-G Ninja] [-DCMAKE_BUILD_TYPE=Release]
#   cmake --build <build>

cmake_minimum_required(VERSION 3.16)
project(Project64 C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Pj64Build)

include(ThirdParty)
include(Common)
include(Project64-rsp-core)
include(Project64-core)
include(Plugins)
include(GLideN64)
if(NOT WIN32)
    include(Project64-headless)
endif()
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    include(Project64)
elseif(WIN32)
    message(STATUS "The Windows UI needs Clang (cmake/toolchain/clang-mingw-w64-x86_64.cmake); not built")
endif()

enable_testing()
include(Project64-test)
