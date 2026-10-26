# Source/Project64-headless: the frontend without a window, with the plugins
# of Plugins.cmake. The program, Plugin/, Config/ and Lang/ form a base
# directory in the build tree, so it runs from there.

add_executable(Project64-headless "${PJ64_SRC}/Project64-headless/main.cpp")
pj64_target(Project64-headless Project64-headless)
target_link_libraries(Project64-headless PRIVATE Project64-core pj64_png)
set_target_properties(Project64-headless PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    # The x64 recompiler addresses the program's globals and heap with 32-bit
    # displacements, as the Windows build allows by turning ASLR off
    set_target_properties(Project64-headless PROPERTIES POSITION_INDEPENDENT_CODE OFF)
    target_link_options(Project64-headless PRIVATE -no-pie)
endif()

# The shipped databases and languages (copied when newer)
file(COPY "${PJ64_ROOT}/Config" "${PJ64_ROOT}/Lang" DESTINATION "${PROJECT_BINARY_DIR}")
