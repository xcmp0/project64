# Bundled third-party libraries (Source/3rdParty), built from their upstream
# project files.

set(PJ64_3RDPARTY "${PJ64_SRC}/3rdParty")

# zlib with minizip (zip ROMs).
set(zlib_exclude "")
if(NOT WIN32)
    list(APPEND zlib_exclude "contrib/minizip/iowin32.c")
endif()
pj64_vcxproj_sources(zlib_sources "${PJ64_3RDPARTY}/zlib/zlib.vcxproj" EXCLUDE ${zlib_exclude})
add_library(pj64_zlib STATIC ${zlib_sources})
target_compile_definitions(pj64_zlib PUBLIC Z_BUFSIZE=46516)
if(NOT WIN32)
    # What zlib's own configure writes into zconf.h; public so every
    # includer sees the same zconf.h.
    target_compile_definitions(pj64_zlib PUBLIC HAVE_UNISTD_H)
endif()
target_include_directories(pj64_zlib PUBLIC "${PJ64_3RDPARTY}/zlib")

# asmjit: x86/x64 and AArch64 code emission for the recompilers.
pj64_vcxproj_sources(asmjit_sources "${PJ64_3RDPARTY}/asmjit/asmjit.vcxproj")
add_library(pj64_asmjit STATIC ${asmjit_sources})
# Empty, as the sources #define it themselves.
target_compile_definitions(pj64_asmjit PUBLIC ASMJIT_STATIC=)
target_include_directories(pj64_asmjit PUBLIC "${PJ64_3RDPARTY}/asmjit/src")
target_link_libraries(pj64_asmjit PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX AND NOT APPLE)
    target_link_libraries(pj64_asmjit PRIVATE rt)
endif()

# SoftFloat 3e (FPU emulation), configured as the MSVC project is:
# 8086 specialization, Win32-SSE2-MinGW platform header.
# msvc_compat.c only supplies __builtin_clz to MSVC. The extF80_* files of
# the project need the 64-bit primitives this configuration leaves out; MSVC
# compiles them with implicit declarations and nothing links them, so the same
# is allowed here (GCC 14 and Clang 16 make it an error by default).
set(softfloat_dir "${PJ64_3RDPARTY}/softfloat-3e")
pj64_vcxproj_sources(softfloat_sources "${softfloat_dir}/softfloat.vcxproj" EXCLUDE msvc_compat.c)
add_library(pj64_softfloat STATIC ${softfloat_sources})
target_compile_options(pj64_softfloat PRIVATE -Wno-implicit-function-declaration)
target_include_directories(pj64_softfloat
    PRIVATE "${softfloat_dir}/source/8086" "${softfloat_dir}/build/Win32-SSE2-MinGW"
    PUBLIC "${softfloat_dir}/source/include")

# 7-Zip C SDK (7z ROMs). This SDK release uses Win32 threads and files; the
# core only opens 7z archives on Windows.
if(WIN32)
    pj64_vcxproj_sources(7zip_sources "${PJ64_3RDPARTY}/7zip/7zip.vcxproj")
    add_library(pj64_7zip STATIC ${7zip_sources})
    target_include_directories(pj64_7zip PUBLIC "${PJ64_3RDPARTY}/7zip")
endif()

# libpng 1.4 (screenshots, texture dumps)
pj64_vcxproj_sources(png_sources "${PJ64_3RDPARTY}/png/png.vcxproj")
add_library(pj64_png STATIC ${png_sources})
target_include_directories(pj64_png PUBLIC "${PJ64_3RDPARTY}/png" "${PJ64_3RDPARTY}")
target_link_libraries(pj64_png PUBLIC pj64_zlib)

if(WIN32)
    # Duktape (the debugger's scripts) and Discord rich presence: Windows UI only
    pj64_vcxproj_sources(duktape_sources "${PJ64_3RDPARTY}/duktape/duktape.vcxproj")
    add_library(pj64_duktape STATIC ${duktape_sources})
    target_include_directories(pj64_duktape PUBLIC "${PJ64_3RDPARTY}/duktape")

    pj64_vcxproj_sources(discord_sources "${PJ64_3RDPARTY}/discord-rpc/discord-rpc.vcxproj")
    add_library(pj64_discord STATIC ${discord_sources})
    target_include_directories(pj64_discord
        PRIVATE "${PJ64_3RDPARTY}/discord-rpc" ${PJ64_INCLUDE_DIRS}
        PUBLIC "${PJ64_3RDPARTY}/discord-rpc/include")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Its rapidjson has template members that only MSVC-style parsing skips
        target_compile_options(pj64_discord PRIVATE -fdelayed-template-parsing)
    endif()
endif()
