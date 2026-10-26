# Cross build for 64-bit Windows with Clang and the MinGW-w64 runtime of the
# posix-thread MinGW GCC (headers, libstdc++, winpthread), from Linux:
#   cmake -S . -B <build> -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/clang-mingw-w64-x86_64.cmake
# The Windows UI needs Clang (WTL relies on MSVC-style template parsing).

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(PJ64_MINGW_TRIPLE x86_64-w64-mingw32)
find_program(CMAKE_C_COMPILER NAMES clang REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES clang++ REQUIRED)
set(CMAKE_C_COMPILER_TARGET ${PJ64_MINGW_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${PJ64_MINGW_TRIPLE})
find_program(CMAKE_RC_COMPILER NAMES ${PJ64_MINGW_TRIPLE}-windres REQUIRED)

# Clang finds a MinGW GCC by itself, but may pick the win32-thread one, which
# has no C++ library; take the posix-thread one's explicitly.
find_program(PJ64_MINGW_GXX NAMES ${PJ64_MINGW_TRIPLE}-g++-posix ${PJ64_MINGW_TRIPLE}-g++ REQUIRED)
execute_process(COMMAND "${PJ64_MINGW_GXX}" -print-libgcc-file-name
    OUTPUT_VARIABLE pj64_libgcc OUTPUT_STRIP_TRAILING_WHITESPACE)
get_filename_component(PJ64_MINGW_GCC_DIR "${pj64_libgcc}" DIRECTORY)
set(CMAKE_CXX_FLAGS_INIT "-nostdinc++ -isystem ${PJ64_MINGW_GCC_DIR}/include/c++ -isystem ${PJ64_MINGW_GCC_DIR}/include/c++/${PJ64_MINGW_TRIPLE} -isystem ${PJ64_MINGW_GCC_DIR}/include/c++/backward")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${PJ64_MINGW_GCC_DIR}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-L${PJ64_MINGW_GCC_DIR}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-L${PJ64_MINGW_GCC_DIR}")

set(CMAKE_FIND_ROOT_PATH /usr/${PJ64_MINGW_TRIPLE})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
