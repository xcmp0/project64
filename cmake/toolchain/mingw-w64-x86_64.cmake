# Cross build for 64-bit Windows with MinGW-w64 (posix threads), from Linux:
#   cmake -S . -B <build> -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/mingw-w64-x86_64.cmake
# MSYS2's MinGW64 shell on Windows needs no toolchain file.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(PJ64_MINGW_TRIPLE x86_64-w64-mingw32)
find_program(CMAKE_C_COMPILER NAMES ${PJ64_MINGW_TRIPLE}-gcc-posix ${PJ64_MINGW_TRIPLE}-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES ${PJ64_MINGW_TRIPLE}-g++-posix ${PJ64_MINGW_TRIPLE}-g++ REQUIRED)
find_program(CMAKE_RC_COMPILER NAMES ${PJ64_MINGW_TRIPLE}-windres REQUIRED)

set(CMAKE_FIND_ROOT_PATH /usr/${PJ64_MINGW_TRIPLE})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
