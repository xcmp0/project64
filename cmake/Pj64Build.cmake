# Shared settings and helpers for the Project64 CMake build.

set(PJ64_ROOT "${PROJECT_SOURCE_DIR}")
set(PJ64_SRC "${PJ64_ROOT}/Source")
set(PJ64_GEN "${PROJECT_BINARY_DIR}/gen")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Static libraries also go into the plugin shared objects.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "This build supports GCC and Clang only; use Project64.sln for MSVC.")
endif()

# MSVC semantics the code relies on: type punning through casts and wrapping
# signed arithmetic.
add_compile_options(-fno-strict-aliasing -fwrapv)

# The MSVC projects define _DEBUG/DEBUG for debug builds and NDEBUG otherwise.
add_compile_definitions($<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Debug>:DEBUG>)

if(WIN32)
    add_compile_definitions(WIN32 _WIN32 _WINDOWS WIN32_LEAN_AND_MEAN UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
endif()
if(MINGW)
    # One file per program and plugin: no MinGW runtime DLLs beside them
    add_link_options(-static)
endif()

# pj64_scan_sources(<out-var>)
#
# The Project64 sources (Source/*, without 3rdParty and GLideN64).
function(pj64_scan_sources out)
    file(GLOB projects LIST_DIRECTORIES true "${PJ64_SRC}/*")
    list(FILTER projects EXCLUDE REGEX "/Source/(3rdParty|GLideN64)$")
    set(sources "")
    foreach(project IN LISTS projects)
        if(IS_DIRECTORY "${project}")
            file(GLOB_RECURSE found LIST_DIRECTORIES false
                "${project}/*.h" "${project}/*.hpp" "${project}/*.c" "${project}/*.cpp" "${project}/*.rc")
            list(APPEND sources ${found})
        endif()
    endforeach()
    set(${out} "${sources}" PARENT_SCOPE)
endfunction()

# pj64_backslash_includes(<dir> <sources>)
#
# The sources mix #include <a/b.h> and #include <a\b.h>. Windows accepts both;
# elsewhere a backslash is part of the file name. For every backslash include
# found under Source, this writes a file of exactly that name into <dir>
# (a\b.h) that includes the real header by its full path, so the real file
# keeps its own directory for its "relative" includes.
function(pj64_backslash_includes dir sources)
    set(names "")
    foreach(source IN LISTS sources)
        file(STRINGS "${source}" lines REGEX "^[ \t]*#[ \t]*include[ \t]*<[^>]*\\\\[^>]*>")
        foreach(line IN LISTS lines)
            string(REGEX REPLACE "^[ \t]*#[ \t]*include[ \t]*<([^>]*)>.*" "\\1" name "${line}")
            list(APPEND names "${name}")
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES names)

    file(REMOVE_RECURSE "${dir}")
    foreach(name IN LISTS names)
        string(REPLACE "\\" "/" path "${name}")
        foreach(root "${PJ64_SRC}" "${PJ64_SRC}/3rdParty" "${PJ64_SRC}/3rdParty/asmjit/src")
            if(EXISTS "${root}/${path}")
                file(WRITE "${dir}/${name}" "#include \"${root}/${path}\"\n")
                break()
            endif()
        endforeach()
    endforeach()
endfunction()

# pj64_find_nocase(<out-var> <root> <relative path>)
#
# The file under <root> whose path matches <relative path> ignoring case, or "".
function(pj64_find_nocase out root path)
    set(${out} "" PARENT_SCOPE)
    string(REPLACE "/" ";" parts "${path}")
    set(current "${root}")
    foreach(part IN LISTS parts)
        if(NOT IS_DIRECTORY "${current}")
            return()
        endif()
        if(part STREQUAL "." OR part STREQUAL "..")
            set(current "${current}/${part}")
            continue()
        endif()
        file(GLOB entries LIST_DIRECTORIES true RELATIVE "${current}" "${current}/*")
        string(TOLOWER "${part}" part_lower)
        set(match "")
        foreach(entry IN LISTS entries)
            string(TOLOWER "${entry}" entry_lower)
            if(entry_lower STREQUAL part_lower)
                set(match "${entry}")
                break()
            endif()
        endforeach()
        if(match STREQUAL "")
            return()
        endif()
        set(current "${current}/${match}")
    endforeach()
    if(NOT IS_DIRECTORY "${current}")
        set(${out} "${current}" PARENT_SCOPE)
    endif()
endfunction()

# pj64_case_includes(<dir> <sources>)
#
# Windows file names ignore case, and the sources rely on it (<Windows.h>,
# <wtl/atlapp.h>, "WinResrc.h"). Cross-compiling on a host that does not, this
# writes a file of the spelling used into <dir> for every include that only
# matches ignoring case, including the real file by its full path. A "quoted"
# include is matched against the system headers only, as a project file of
# that name may exist in more than one directory.
function(pj64_case_includes dir sources)
    set(project_roots "${PJ64_SRC}" "${PJ64_SRC}/3rdParty" "${PJ64_SRC}/3rdParty/asmjit/src")
    set(system_roots ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    set(angle "")
    set(quoted "")
    foreach(source IN LISTS sources)
        get_filename_component(source_dir "${source}" DIRECTORY)
        file(STRINGS "${source}" lines REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"][^>\"\\\\]+[>\"]")
        foreach(line IN LISTS lines)
            string(REGEX REPLACE "^[ \t]*#[ \t]*include[ \t]*([<\"])([^>\"]+)[>\"].*" "\\1" kind "${line}")
            string(REGEX REPLACE "^[ \t]*#[ \t]*include[ \t]*[<\"]([^>\"]+)[>\"].*" "\\1" name "${line}")
            if(kind STREQUAL "<")
                list(APPEND angle "${name}")
            elseif(NOT EXISTS "${source_dir}/${name}")
                list(APPEND quoted "${name}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES angle)
    list(REMOVE_DUPLICATES quoted)

    file(REMOVE_RECURSE "${dir}")
    foreach(kind angle quoted)
        if(kind STREQUAL "angle")
            set(search ${project_roots} ${system_roots})
        else()
            set(search ${system_roots})
        endif()
        foreach(name IN LISTS ${kind})
            set(found FALSE)
            foreach(root IN LISTS project_roots system_roots)
                if(EXISTS "${root}/${name}")
                    set(found TRUE)
                    break()
                endif()
            endforeach()
            if(found)
                continue()
            endif()
            foreach(root IN LISTS search)
                pj64_find_nocase(real "${root}" "${name}")
                if(NOT real STREQUAL "")
                    file(WRITE "${dir}/${name}" "#include \"${real}\"\n")
                    break()
                endif()
            endforeach()
        endforeach()
    endforeach()
endfunction()

# pj64_windows_includes(<dir> <sources> <root>...)
#
# The case-blind, backslash-tolerant includes of sources written for Windows
# only (no shared roots, unlike the two functions above): for every #include
# whose name has a backslash or matches only ignoring case, a file of that
# exact name in <dir> includes the real file. The sources and every header
# under the roots are read. "Quoted" names are looked up from the including
# file's directory first, then in the roots; <angled> names in the roots and
# the system headers.
function(pj64_windows_includes dir sources)
    set(roots ${ARGN})
    set(system_roots ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    set(files ${sources})
    foreach(root IN LISTS roots)
        file(GLOB_RECURSE headers "${root}/*.h" "${root}/*.hpp" "${root}/*.inl")
        list(APPEND files ${headers})
    endforeach()
    list(REMOVE_DUPLICATES files)
    file(REMOVE_RECURSE "${dir}")
    set(done "")
    foreach(source IN LISTS files)
        get_filename_component(source_dir "${source}" DIRECTORY)
        file(STRINGS "${source}" lines REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"][^>\"]+[>\"]")
        foreach(line IN LISTS lines)
            string(REGEX REPLACE "^[ \t]*#[ \t]*include[ \t]*([<\"])([^>\"]+)[>\"].*" "\\1" kind "${line}")
            string(REGEX REPLACE "^[ \t]*#[ \t]*include[ \t]*[<\"]([^>\"]+)[>\"].*" "\\1" name "${line}")
            if(name IN_LIST done)
                continue()
            endif()
            string(REPLACE "\\" "/" path "${name}")
            set(search ${roots} ${system_roots})
            if(kind STREQUAL "\"")
                list(PREPEND search "${source_dir}")
            endif()
            set(real "")
            foreach(root IN LISTS search)
                if(EXISTS "${root}/${path}")
                    set(real "${root}/${path}")
                    break()
                endif()
            endforeach()
            set(exact TRUE)
            if(real STREQUAL "")
                set(exact FALSE)
                foreach(root IN LISTS search)
                    pj64_find_nocase(real "${root}" "${path}")
                    if(NOT real STREQUAL "")
                        break()
                    endif()
                endforeach()
            endif()
            if(real STREQUAL "")
                continue()
            endif()
            list(APPEND done "${name}")
            # Clang with -fms-extensions turns the backslashes into slashes
            # itself; without, they stay part of the name
            if(NOT path STREQUAL name)
                file(WRITE "${dir}/${name}" "#include \"${real}\"\n")
            endif()
            if(NOT exact)
                file(WRITE "${dir}/${path}" "#include \"${real}\"\n")
            endif()
        endforeach()
    endforeach()
endfunction()

# Include roots shared by every Project64 target (PropertySheets/Platform.props).
# Generated headers come first, so a Version.h left in Source by an MSVC build
# does not shadow the one generated here.
set(PJ64_INCLUDE_DIRS "${PJ64_GEN}/Source" "${PJ64_SRC}" "${PJ64_SRC}/3rdParty")
if(NOT CMAKE_HOST_WIN32)
    pj64_scan_sources(pj64_sources)
    pj64_backslash_includes("${PJ64_GEN}/backslash" "${pj64_sources}")
    list(APPEND PJ64_INCLUDE_DIRS "${PJ64_GEN}/backslash")
    if(WIN32)
        pj64_case_includes("${PJ64_GEN}/case" "${pj64_sources}")
        list(APPEND PJ64_INCLUDE_DIRS "${PJ64_GEN}/case")
    endif()
endif()
if(NOT WIN32)
    # Stand-ins for MSVC-only headers (cmake/include/msvc).
    list(APPEND PJ64_INCLUDE_DIRS "${PJ64_ROOT}/cmake/include/msvc")
endif()

# pj64_target(<target> <project dir>)
#
# Settings for a Project64 (not third-party) target: its project directory
# first (".\" in the MSVC projects), then the shared include roots, and the
# forced compatibility header (cmake/include/Pj64Compat.h).
function(pj64_target target project_dir)
    target_include_directories(${target} PRIVATE "${PJ64_SRC}/${project_dir}")
    target_include_directories(${target} PUBLIC ${PJ64_INCLUDE_DIRS})
    target_compile_options(${target} PRIVATE
        "SHELL:-include \"${PJ64_ROOT}/cmake/include/Pj64Compat.h\""
        # What MSVC only warns about: register, narrowing in braces and case labels
        $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-Wno-register -Wno-c++11-narrowing>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-register -Wno-narrowing>)
    pj64_utf16_overlay(${target} "${project_dir}")
endfunction()

# pj64_utf16_overlay(<target> <directory>...)
#
# The UTF-16 headers of the directories (under Source): <target> reads their
# UTF-8 copies in their place, through a Clang VFS overlay.
function(pj64_utf16_overlay target)
    set(roots "")
    foreach(dir IN LISTS ARGN)
        pj64_utf16_headers(converted "${dir}")
        if(NOT converted)
            continue()
        endif()
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(FATAL_ERROR "${dir} has UTF-16 headers (${converted}), which only Clang can read (through a VFS overlay)")
        endif()
        set(files "")
        foreach(name IN LISTS converted)
            list(APPEND files "{'name': '${name}', 'type': 'file', 'external-contents': '${PJ64_GEN}/Source/${dir}/${name}'}")
        endforeach()
        list(JOIN files ", " files)
        list(APPEND roots "{'name': '${PJ64_SRC}/${dir}', 'type': 'directory', 'contents': [${files}]}")
    endforeach()
    if(roots)
        list(JOIN roots ", " roots)
        set(overlay "${PJ64_GEN}/vfs/${target}.yaml")
        file(WRITE "${overlay}" "{'version': 0, 'roots': [${roots}]}\n")
        target_compile_options(${target} PRIVATE "SHELL:-ivfsoverlay \"${overlay}\"")
    endif()
endfunction()

# pj64_utf16_text(<out-var> <file>)
#
# The text of a file, decoded from UTF-16 (little-endian, with a byte order
# mark) when it is saved that way, as Visual Studio does with some resource
# scripts and headers. GCC, Clang and windres read only 8-bit text.
function(pj64_utf16_text out file)
    file(READ "${file}" bom LIMIT 2 HEX)
    if(bom STREQUAL "fffe")
        file(STRINGS "${file}" text ENCODING UTF-16LE NEWLINE_CONSUME)
    else()
        file(READ "${file}" text)
    endif()
    set(${out} "${text}" PARENT_SCOPE)
endfunction()

# pj64_utf16_headers(<out-var> <directory>)
#
# Writes UTF-8 copies of the UTF-16 headers of <directory> (under Source) to
# the same place in the generated tree and returns their names.
function(pj64_utf16_headers out dir)
    file(GLOB headers "${PJ64_SRC}/${dir}/*.h")
    set(names "")
    foreach(header IN LISTS headers)
        file(READ "${header}" bom LIMIT 2 HEX)
        if(bom STREQUAL "fffe")
            get_filename_component(name "${header}" NAME)
            pj64_utf16_text(text "${header}")
            pj64_write_if_changed("${PJ64_GEN}/Source/${dir}/${name}" "${text}")
            set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${header}")
            list(APPEND names "${name}")
        endif()
    endforeach()
    set(${out} "${names}" PARENT_SCOPE)
endfunction()

# pj64_write_if_changed(<file> <text>)
function(pj64_write_if_changed file text)
    set(old "")
    if(EXISTS "${file}")
        file(READ "${file}" old)
    endif()
    if(NOT old STREQUAL text)
        file(WRITE "${file}" "${text}")
    endif()
endfunction()

# pj64_rc(<out-var> <rc file>)
#
# Writes the copy of a resource script that windres reads and returns it: the
# script decoded from UTF-16 when it is saved that way, without what windres
# does not read (DLGINIT), and when cross-compiling on a host that is not
# Windows, with the Windows-style names of its icons, bitmaps and cursors
# ("Icons\\PJ64.ICO") replaced by the matching files (case-blind). The copy
# sits in the generated tree where the script sits in Source, next to the
# generated Version.h and the UTF-8 copies of UTF-16 headers, so its quoted
# includes find those first; the script's own directory follows on its include
# path.
function(pj64_rc out rc)
    get_filename_component(rc_dir "${rc}" DIRECTORY)
    get_filename_component(rc_name "${rc}" NAME)
    file(RELATIVE_PATH rc_rel "${PJ64_SRC}" "${rc_dir}")
    pj64_utf16_text(text "${rc}")
    pj64_utf16_headers(converted "${rc_rel}")
    if(NOT CMAKE_HOST_WIN32)
        string(REGEX MATCHALL "[ \t](ICON|BITMAP|CURSOR|PNG|RCDATA|HTML)[ \t]+(DISCARDABLE[ \t]+)?\"[^\"\n]+\"" refs "${text}")
        foreach(ref IN LISTS refs)
            string(REGEX REPLACE ".*\"([^\"]+)\"$" "\\1" name "${ref}")
            string(REPLACE "\\\\" "/" path "${name}")
            string(REPLACE "\\" "/" path "${path}")
            pj64_find_nocase(real "${rc_dir}" "${path}")
            if(NOT real STREQUAL "")
                string(REPLACE "\"${name}\"" "\"${real}\"" text "${text}")
            endif()
        endforeach()
    endif()
    # DLGINIT blocks (combo box items for MFC and ActiveX hosts) are MSVC-only;
    # WTL dialogs never read them
    string(FIND "${text}" "DLGINIT" pos)
    while(pos GREATER -1)
        string(SUBSTRING "${text}" 0 ${pos} head)
        string(FIND "${head}" "\n" line_start REVERSE)
        math(EXPR line_start "${line_start} + 1")
        string(SUBSTRING "${text}" ${pos} -1 tail)
        string(FIND "${tail}" "\nEND" end)
        math(EXPR end "${pos} + ${end} + 4")
        string(SUBSTRING "${text}" 0 ${line_start} before)
        string(SUBSTRING "${text}" ${end} -1 after)
        set(text "${before}${after}")
        string(FIND "${text}" "DLGINIT" pos)
    endwhile()
    # Resource scripts it includes get the same treatment, and the include
    # names their copy (written Windows-style: "GLideNUI-wtl\\GLideNUI.rc")
    string(REGEX MATCHALL "#include[ \t]+\"[^\"\n]+\\.rc\"" nested "${text}")
    foreach(line IN LISTS nested)
        string(REGEX REPLACE ".*\"([^\"]+)\"$" "\\1" name "${line}")
        string(REPLACE "\\" "/" path "${name}")
        pj64_find_nocase(real "${rc_dir}" "${path}")
        if(NOT real STREQUAL "")
            pj64_rc(nested_copy "${real}")
            string(REPLACE "\"${name}\"" "\"${nested_copy}\"" text "${text}")
        endif()
    endforeach()
    set(copy "${PJ64_GEN}/Source/${rc_rel}/${rc_name}")
    pj64_write_if_changed("${copy}" "${text}")
    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${rc}")
    set_property(SOURCE "${copy}" APPEND PROPERTY INCLUDE_DIRECTORIES "${rc_dir}")
    set(${out} "${copy}" PARENT_SCOPE)
endfunction()

# pj64_wtl(<target>)
#
# A target that uses WTL: WTL 10 on ReactOS's ATL (Source/3rdParty/ReactOS-ATL)
# with the additions WTL needs (Source/3rdParty/atlcompat). Needs Clang: WTL
# leans on MSVC's template parsing. No ActiveX hosting; Windows Vista APIs.
function(pj64_wtl target)
    target_include_directories(${target} BEFORE PRIVATE
        "${PJ64_SRC}/3rdParty/atlcompat" "${PJ64_SRC}/3rdParty/ReactOS-ATL")
    target_compile_definitions(${target} PRIVATE _ATL_NO_HOSTING _WIN32_WINNT=0x0600 WINVER=0x0600)
    target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fdelayed-template-parsing -fms-extensions>)
endfunction()

# The functions of the plugin specs (Source/Project64-plugin-spec), by plugin
# directory, and those of the settings API (Source/Settings)
set(PJ64_EXPORTS_ALL GetDllInfo CloseDLL DllAbout DllConfig DllTest RomOpen RomClosed PluginLoaded
    SetSettingInfo SetSettingInfo2 SetSettingInfo3 SetSettingNotificationInfo SetPluginNotification)
set(PJ64_EXPORTS_RSP InitiateRSP DoRspCycles EnableDebugging GetRspDebugInfo InitiateRSPDebugger)
set(PJ64_EXPORTS_GFX InitiateGFX CaptureScreen ChangeWindow DrawScreen DrawStatus MoveScreen ProcessDList
    ProcessRDPList ShowCFB SoftReset UpdateScreen ViStatusChanged ViWidthChanged ReadScreen ReadScreen2
    DllCrtFree GetVideoSize FBRead FBWrite FBGetFrameBufferInfo GetRomBrowserMenu OnRomBrowserMenuItem)
set(PJ64_EXPORTS_Audio InitiateAudio AiDacrateChanged AiLenChanged AiReadLength AiUpdate ProcessAList)
set(PJ64_EXPORTS_Input InitiateControllers ControllerCommand GetKeys ReadController RumbleCommand
    WM_KeyDown WM_KeyUp WM_KillFocus EmulationPaused)

# pj64_plugin(<target> <output name> <plugin directory> [<subdirectory>])
#
# A plugin library, written to <build>/Plugin/<plugin directory>/<output name>,
# or into a subdirectory of it (a plugin with files of its own, as installed).
# An ELF plugin exports the spec functions of its kind and nothing else: every
# plugin links its own Common and Settings, whose globals must not bind across
# plugins and the program.
function(pj64_plugin target output_name plugin_dir)
    set(output_dir "${PROJECT_BINARY_DIR}/Plugin/${plugin_dir}")
    if(ARGC GREATER 3)
        set(output_dir "${output_dir}/${ARGV3}")
    endif()
    set_target_properties(${target} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${output_name}"
        RUNTIME_OUTPUT_DIRECTORY "${output_dir}"
        LIBRARY_OUTPUT_DIRECTORY "${output_dir}")
    if(NOT WIN32 AND NOT APPLE)
        list(JOIN PJ64_EXPORTS_ALL ";\n        " all)
        list(JOIN PJ64_EXPORTS_${plugin_dir} ";\n        " kind)
        set(map "${PJ64_GEN}/exports/${target}.map")
        pj64_write_if_changed("${map}" "{\n    global:\n        ${all};\n        ${kind};\n    local: *;\n};\n")
        target_link_options(${target} PRIVATE "LINKER:--version-script=${map}" "LINKER:--no-undefined")
        set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${map}")
    endif()
endfunction()

# pj64_vcxproj_sources(<out-var> <vcxproj> [CONFIGURATION <name>] [PLATFORM <name>] [EXCLUDE <path>...])
#
# Reads the <ClCompile Include="..."> items of an upstream project file and
# returns them as absolute paths, without those it excludes from every build
# or, given CONFIGURATION and PLATFORM, from that build (conditions on
# '$(Configuration)' or '$(Platform)' joined by Or).
# EXCLUDE takes paths as written in the project file (either slash, any case).
# The project files were written for a case-blind file system: an item not
# found as written is matched case-blind against the files on disk and
# returned with the on-disk spelling.
function(pj64_vcxproj_sources out vcxproj)
    cmake_parse_arguments(ARG "" "CONFIGURATION;PLATFORM" "EXCLUDE" ${ARGN})
    get_filename_component(dir "${vcxproj}" DIRECTORY)

    set(excluded "")
    foreach(item IN LISTS ARG_EXCLUDE)
        string(REPLACE "\\" "/" item "${item}")
        string(TOLOWER "${item}" item)
        list(APPEND excluded "${item}")
    endforeach()

    # Items with an unconditional <ExcludedFromBuild>true</ExcludedFromBuild> are skipped
    file(READ "${vcxproj}" project)
    # A changed project (files added upstream) configures again
    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${vcxproj}")
    string(REGEX MATCHALL "<ClCompile Include=\"[^\"]+\">[^<]*<ExcludedFromBuild>true</ExcludedFromBuild>" skipped "${project}")
    foreach(item IN LISTS skipped)
        string(REGEX REPLACE "<ClCompile Include=\"([^\"]+)\".*" "\\1" item "${item}")
        string(REPLACE "\\" "/" item "${item}")
        string(TOLOWER "${item}" item)
        list(APPEND excluded "${item}")
    endforeach()

    # So are those excluded for the given configuration or platform: conditions
    # of the form '$(Configuration)'=='X' Or '$(Platform)'=='Y' ...
    string(REGEX MATCHALL "<ClCompile Include=\"[^\"]+\">[^<]*<ExcludedFromBuild Condition=\"[^\"]*\">true</ExcludedFromBuild>" conditional "${project}")
    foreach(item IN LISTS conditional)
        string(REGEX REPLACE ".*Condition=\"([^\"]*)\".*" "\\1" condition "${item}")
        set(match FALSE)
        if(DEFINED ARG_CONFIGURATION AND condition MATCHES "'\\$\\(Configuration\\)'=='${ARG_CONFIGURATION}'")
            set(match TRUE)
        endif()
        if(DEFINED ARG_PLATFORM AND condition MATCHES "'\\$\\(Platform\\)'=='${ARG_PLATFORM}'")
            set(match TRUE)
        endif()
        if(match)
            string(REGEX REPLACE "<ClCompile Include=\"([^\"]+)\".*" "\\1" item "${item}")
            string(REPLACE "\\" "/" item "${item}")
            string(TOLOWER "${item}" item)
            list(APPEND excluded "${item}")
        endif()
    endforeach()

    file(STRINGS "${vcxproj}" lines REGEX "<ClCompile Include=\"[^\"]+\"")
    set(files "")
    foreach(line IN LISTS lines)
        string(REGEX REPLACE ".*<ClCompile Include=\"([^\"]+)\".*" "\\1" item "${line}")
        string(REPLACE "\\" "/" item "${item}")
        string(TOLOWER "${item}" item_lower)
        if(item_lower IN_LIST excluded)
            continue()
        endif()
        if(EXISTS "${dir}/${item}")
            list(APPEND files "${dir}/${item}")
        else()
            pj64_find_nocase(real "${dir}" "${item}")
            if(real STREQUAL "")
                message(FATAL_ERROR "${vcxproj}: ${item} not found")
            endif()
            list(APPEND files "${real}")
        endif()
    endforeach()
    set(${out} "${files}" PARENT_SCOPE)
endfunction()

# pj64_version_header(<target> <project dir>)
#
# Generates <gen>/Source/<project dir>/Version.h from the project's
# Version.h.in on every build (git build number, revision and dirty flag, as
# Source/UpdateVersion does) and makes <target> depend on it. The file is only
# rewritten when its content changes.
function(pj64_version_header target project_dir)
    set(in "${PJ64_SRC}/${project_dir}/Version.h.in")
    set(out "${PJ64_GEN}/Source/${project_dir}/Version.h")
    string(MAKE_C_IDENTIFIER "${project_dir}" id)
    add_custom_target(pj64_version_${id}
        COMMAND "${CMAKE_COMMAND}" "-DIN=${in}" "-DOUT=${out}" "-DREPO=${PJ64_ROOT}"
                -P "${PJ64_ROOT}/cmake/Pj64Version.cmake"
        BYPRODUCTS "${out}"
        COMMENT "Generating ${project_dir}/Version.h"
        VERBATIM)
    add_dependencies(${target} pj64_version_${id})
    # Sources in the project include it as "Version.h".
    target_include_directories(${target} PRIVATE "${PJ64_GEN}/Source/${project_dir}")
endfunction()
