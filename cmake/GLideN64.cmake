# Source/GLideN64/Source (submodule): GLideN64 as the video plugin, built from
# the source list of its Visual Studio project (the WTL configuration). On
# Windows it is that plugin, WGL with the GLideNUI-wtl dialogs on the ATL
# layer of pj64_wtl (Clang only); off Windows it renders offscreen through
# EGL (src/Graphics/OpenGLContext/egl, src/headless), with no configuration
# UI, for the headless frontend. Neither has texture packs (GLideNHQ) or
# on-screen text (FreeType). It goes to Plugin/GFX/GLideN64/, as installed.

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

if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(gln_rel "GLideN64/Source/src")
    pj64_vcxproj_sources(gliden64_ui_sources "${GLIDEN64_DIR}/projects/msvc/GLideNUI-wtl.vcxproj"
        CONFIGURATION Release PLATFORM x64)
    list(FILTER gliden64_sources EXCLUDE REGEX "/src/(TextDrawer|iob)\\.cpp$")
    list(APPEND gliden64_sources
        "${gln}/TextDrawerStub.cpp"
        "${gln}/TxFilterStub.cpp"
        "${gln}/osal/osal_files_win32.c"
        "${gln}/osal/osal_keys_win.c")

    # Screenshots: the source names the libpng 1.6 header of the libraries the
    # MSVC build takes from vcpkg; a copy builds against Project64's libpng 1.4,
    # whose functions it calls the same way (and gets <stdint.h> as MSVC's
    # headers give it)
    set(screenshot "${gln}/windows/ScreenShot.cpp")
    list(FILTER gliden64_sources EXCLUDE REGEX "/src/windows/ScreenShot\\.cpp$")
    file(READ "${screenshot}" text)
    string(REPLACE "#include \"../GLideNHQ/inc/png.h\"" "#include <stdint.h>\n#include <png.h>" text "${text}")
    pj64_write_if_changed("${PJ64_GEN}/GLideN64/ScreenShot.cpp" "${text}")
    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${screenshot}")
    list(APPEND gliden64_sources "${PJ64_GEN}/GLideN64/ScreenShot.cpp")
    set(gliden64_includes "${PJ64_GEN}/GLideN64" "${gln}" "${gln}/inc" "${gln}/osal")
    if(NOT CMAKE_HOST_WIN32)
        pj64_windows_includes("${PJ64_GEN}/GLideN64/windows" "${gliden64_sources};${gliden64_ui_sources}"
            "${gln}" "${gln}/inc" "${gln}/osal")
        list(APPEND gliden64_includes "${PJ64_GEN}/GLideN64/windows")
    endif()
    set(gliden64_defines OS_WINDOWS UNICODE _UNICODE NDEBUG WIN64 _CRT_SECURE_NO_WARNINGS WTL_UI
        TXFILTER_LIB GL_USE_UNIFORMBLOCK ZILMAR_API NOHQ NO_OSD)

    # GLideN64's projects keep the whole <windows.h> (commdlg.h and the like)
    set(gliden64_options -UWIN32_LEAN_AND_MEAN -Wno-register -Wno-c++11-narrowing)

    # The WTL 8.1 copy looks up names of dependent base classes (m_hWnd) as
    # MSVC does; Clang does so in MSVC compatibility, which here keeps the
    # GCC-compatible MinGW headers working with a GNU and an MSVC 2019 version.
    add_library(GLideNUI-wtl STATIC ${gliden64_ui_sources})
    target_include_directories(GLideNUI-wtl PRIVATE ${gliden64_includes} "${gln}/GLideNUI-wtl")
    target_compile_definitions(GLideNUI-wtl PRIVATE ${gliden64_defines} _WINDOWS WIN32)
    target_compile_options(GLideNUI-wtl PRIVATE ${gliden64_options}
        -fms-compatibility -fms-compatibility-version=19.29 -fgnuc-version=4.2.1
        "SHELL:-include \"${PJ64_ROOT}/cmake/include/GLideNUICompat.h\"")
    pj64_wtl(GLideNUI-wtl)
    pj64_utf16_overlay(GLideNUI-wtl "${gln_rel}" "${gln_rel}/GLideNUI-wtl")

    pj64_rc(gliden64_rc "${gln}/GLideN64.rc")
    set_property(SOURCE "${gliden64_rc}" APPEND PROPERTY COMPILE_DEFINITIONS WTL_UI)
    add_library(GLideN64 SHARED ${gliden64_sources} "${gliden64_rc}")
    target_include_directories(GLideN64 PRIVATE ${gliden64_includes})
    target_compile_definitions(GLideN64 PRIVATE ${gliden64_defines} _USRDLL)
    target_compile_options(GLideN64 PRIVATE ${gliden64_options})
    pj64_utf16_overlay(GLideN64 "${gln_rel}")
    target_link_libraries(GLideN64 PRIVATE GLideNUI-wtl pj64_png Threads::Threads opengl32 gdi32 winmm comctl32 comdlg32 ole32 shell32 uxtheme dwrite)
    pj64_plugin(GLideN64 GLideN64 GFX GLideN64)
    # The settings by game, read from the plugin's directory
    add_custom_command(TARGET GLideN64 POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${GLIDEN64_DIR}/ini/GLideN64.custom.ini" "$<TARGET_FILE_DIR:GLideN64>")
elseif(WIN32)
    message(STATUS "GLideN64 needs Clang on Windows (GLideNUI-wtl); not built")
else()
    # The Windows glue and UI give way to the offscreen display; no MSVC CRT
    # shim (iob)
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
    pj64_plugin(GLideN64 GLideN64 GFX GLideN64)
endif()
