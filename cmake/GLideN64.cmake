# Source/GLideN64/Source (submodule): GLideN64 as the video plugin, built from
# the source list of its Visual Studio project (the WTL configuration). Off
# Windows it renders offscreen through EGL (src/Graphics/OpenGLContext/egl,
# src/headless), with no configuration UI, for the headless frontend.

set(GLIDEN64_DIR "${PJ64_SRC}/GLideN64/Source")
if(NOT EXISTS "${GLIDEN64_DIR}/src/GLideN64.cpp")
    message(STATUS "GLideN64 not built: git submodule update --init Source/GLideN64/Source")
    return()
endif()
set(gln "${GLIDEN64_DIR}/src")

pj64_vcxproj_sources(gliden64_sources "${GLIDEN64_DIR}/projects/msvc/GLideN64.vcxproj"
    CONFIGURATION Release_wtl PLATFORM x64)

# Revision.h, as getRevision.sh writes it
set(gliden64_revision "")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${GLIDEN64_DIR}" rev-parse --short HEAD
        OUTPUT_VARIABLE gliden64_revision OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()
if(gliden64_revision STREQUAL "")
    set(revision_text "\n")
else()
    set(revision_text "#define PLUGIN_REVISION \"${gliden64_revision}\"\n#define PLUGIN_REVISION_W L\"${gliden64_revision}\"\n")
endif()
pj64_write_if_changed("${PJ64_GEN}/GLideN64/Revision.h" "${revision_text}")

if(NOT WIN32)
    # The Windows glue and UI give way to the offscreen display; no texture
    # packs (GLideNHQ), no on-screen text (FreeType), no MSVC CRT shim (iob)
    list(FILTER gliden64_sources EXCLUDE REGEX "/src/(windows|GLideNUI|Graphics/OpenGLContext/windows)/")
    list(FILTER gliden64_sources EXCLUDE REGEX "/src/(TextDrawer|iob)\\.cpp$")
    list(APPEND gliden64_sources
        "${gln}/headless/GLideN64_headless.cpp"
        "${gln}/Graphics/OpenGLContext/egl/EglContext.cpp"
        "${gln}/Graphics/OpenGLContext/egl/egl_DisplayWindow.cpp"
        "${gln}/mupenplus/MemoryStatus_mupenplus.cpp"
        "${gln}/TextDrawerStub.cpp"
        "${gln}/TxFilterStub.cpp"
        "${gln}/osal/osal_files_unix.c"
        "${gln}/osal/osal_keys_unix.c")

    add_library(GLideN64 SHARED ${gliden64_sources})
    target_include_directories(GLideN64 PRIVATE "${PJ64_GEN}/GLideN64" "${gln}" "${gln}/inc" "${gln}/osal")
    target_compile_definitions(GLideN64 PRIVATE OS_LINUX EGL NOHQ NO_OSD)
    target_compile_options(GLideN64 PRIVATE -fvisibility=hidden -Wno-register
        $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-Wno-c++11-narrowing> $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-narrowing>)
    target_link_libraries(GLideN64 PRIVATE EGL Threads::Threads ${CMAKE_DL_LIBS})
    pj64_plugin(GLideN64 GLideN64 GFX)
endif()
