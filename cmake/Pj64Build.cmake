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

# pj64_backslash_includes(<dir>)
#
# The sources mix #include <a/b.h> and #include <a\b.h>. Windows accepts both;
# elsewhere a backslash is part of the file name. For every backslash include
# found under Source, this writes a file of exactly that name into <dir>
# (a\b.h) that includes the real header by its full path, so the real file
# keeps its own directory for its "relative" includes.
function(pj64_backslash_includes dir)
    file(GLOB projects LIST_DIRECTORIES true "${PJ64_SRC}/*")
    list(FILTER projects EXCLUDE REGEX "/Source/(3rdParty|GLideN64)$")
    set(sources "")
    foreach(project IN LISTS projects)
        if(IS_DIRECTORY "${project}")
            file(GLOB_RECURSE found LIST_DIRECTORIES false
                "${project}/*.h" "${project}/*.hpp" "${project}/*.c" "${project}/*.cpp")
            list(APPEND sources ${found})
        endif()
    endforeach()
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

# Include roots shared by every Project64 target (PropertySheets/Platform.props).
# Generated headers come first, so a Version.h left in Source by an MSVC build
# does not shadow the one generated here.
set(PJ64_INCLUDE_DIRS "${PJ64_GEN}/Source" "${PJ64_SRC}" "${PJ64_SRC}/3rdParty")
if(NOT CMAKE_HOST_WIN32)
    pj64_backslash_includes("${PJ64_GEN}/backslash")
    list(APPEND PJ64_INCLUDE_DIRS "${PJ64_GEN}/backslash")
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
        "SHELL:-include \"${PJ64_ROOT}/cmake/include/Pj64Compat.h\"")
endfunction()

# pj64_vcxproj_sources(<out-var> <vcxproj> [EXCLUDE <path>...])
#
# Reads the <ClCompile Include="..."> items of an upstream project file and
# returns them as absolute paths. EXCLUDE takes paths as written in the
# project file (either slash, any case). The project files were written for a
# case-blind file system: an item not found as written is matched case-blind
# against the files on disk and returned with the on-disk spelling.
function(pj64_vcxproj_sources out vcxproj)
    cmake_parse_arguments(ARG "" "" "EXCLUDE" ${ARGN})
    get_filename_component(dir "${vcxproj}" DIRECTORY)

    set(excluded "")
    foreach(item IN LISTS ARG_EXCLUDE)
        string(REPLACE "\\" "/" item "${item}")
        string(TOLOWER "${item}" item)
        list(APPEND excluded "${item}")
    endforeach()

    file(STRINGS "${vcxproj}" lines REGEX "<ClCompile Include=\"[^\"]+\"")
    set(files "")
    set(disk_listed FALSE)
    foreach(line IN LISTS lines)
        string(REGEX REPLACE ".*<ClCompile Include=\"([^\"]+)\".*" "\\1" item "${line}")
        string(REPLACE "\\" "/" item "${item}")
        string(TOLOWER "${item}" item_lower)
        if(item_lower IN_LIST excluded)
            continue()
        endif()
        if(NOT EXISTS "${dir}/${item}")
            if(NOT disk_listed)
                file(GLOB_RECURSE disk LIST_DIRECTORIES false RELATIVE "${dir}" "${dir}/*")
                string(TOLOWER "${disk}" disk_lower)
                set(disk_listed TRUE)
            endif()
            list(FIND disk_lower "${item_lower}" index)
            if(index LESS 0)
                message(FATAL_ERROR "${vcxproj}: ${item} not found")
            endif()
            list(GET disk ${index} item)
        endif()
        list(APPEND files "${dir}/${item}")
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
