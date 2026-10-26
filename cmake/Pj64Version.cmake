# Writes Version.h from Version.h.in with the substitutions of
# Source/UpdateVersion: build number (commit count), git revision, dirty flag
# and build year. Without git the template values are kept, except the year.
#
#   cmake -DIN=<Version.h.in> -DOUT=<Version.h> -DREPO=<checkout> -P Pj64Version.cmake

set(build "")
set(revision "")
set(revision_short "")
set(dirty FALSE)

find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE AND EXISTS "${REPO}/.git")
    macro(pj64_git var)
        execute_process(COMMAND "${GIT_EXECUTABLE}" ${ARGN}
            WORKING_DIRECTORY "${REPO}"
            OUTPUT_VARIABLE ${var}
            RESULT_VARIABLE git_result
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        if(NOT git_result EQUAL 0)
            set(${var} "")
        endif()
    endmacro()
    pj64_git(build rev-list --count HEAD)
    pj64_git(revision rev-parse HEAD)
    pj64_git(revision_short rev-parse --short HEAD)
    pj64_git(diff_stat diff --stat)
    if(NOT diff_stat STREQUAL "")
        set(dirty TRUE)
    endif()
endif()

file(READ "${IN}" text)
string(REPLACE "\r" "" text "${text}")
string(TIMESTAMP year "%Y" UTC)

macro(pj64_define name value)
    string(REGEX REPLACE "#define ${name}[ \t][^\n]*" "#define ${name} ${value}" text "${text}")
endmacro()

pj64_define(VERSION_BUILD_YEAR "${year}")
if(build MATCHES "^[1-9][0-9]*$")
    pj64_define(VERSION_BUILD "${build}")
endif()
if(NOT revision STREQUAL "")
    if(dirty)
        pj64_define(GIT_VERSION "\"${revision_short}-Dirty\"")
        pj64_define(GIT_DIRTY "\"Dirty\"")
    else()
        pj64_define(GIT_VERSION "\"${revision_short}\"")
        pj64_define(GIT_DIRTY "\"\"")
    endif()
    pj64_define(GIT_REVISION "\"${revision}\"")
    pj64_define(GIT_REVISION_SHORT "\"${revision_short}\"")
endif()

set(old "")
if(EXISTS "${OUT}")
    file(READ "${OUT}" old)
endif()
if(NOT old STREQUAL text)
    file(WRITE "${OUT}" "${text}")
endif()
