# Source/Project64-test: core tests, run by ctest.

add_executable(Project64-test
    "${PJ64_SRC}/Project64-test/main.cpp"
    "${PJ64_SRC}/Project64-test/GameKeys.cpp")
pj64_target(Project64-test Project64-test)
target_link_libraries(Project64-test PRIVATE Project64-core)

add_test(NAME game-keys COMMAND Project64-test game-keys "${PROJECT_BINARY_DIR}/test/game-keys")
add_test(NAME base-dir COMMAND Project64-test base-dir "${PROJECT_BINARY_DIR}/test/base-dir")

if(TARGET Project64-headless)
    # The headless frontend on the test ROM (a boot loop) with the headless
    # plugins, from the build tree as its base directory
    set(headless_dir "${PROJECT_BINARY_DIR}/test/headless")
    add_test(NAME headless-rom COMMAND Project64-test make-rom "${headless_dir}/loop.z64")
    set_tests_properties(headless-rom PROPERTIES FIXTURES_SETUP headless)
    # The x64 recompiler on its own and checked against the interpreter (sync)
    set(headless_cpus interpreter)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        list(APPEND headless_cpus recompiler sync)
    endif()
    foreach(cpu IN LISTS headless_cpus)
        add_test(NAME headless-${cpu} COMMAND Project64-headless --frames 120 --cpu ${cpu}
            --gfx GFX/Project64-Video-Null.so --rsp RSP/Project64-RSP.so
            --audio Audio/Project64-Audio-Wav.so --input Input/Project64-Input-Script.so
            --wav "${headless_dir}/audio-${cpu}.wav" --dump "${headless_dir}" "${headless_dir}/loop.z64")
        # One base directory (the build tree): one run at a time
        set_tests_properties(headless-${cpu} PROPERTIES
            FIXTURES_REQUIRED headless RESOURCE_LOCK headless-basedir TIMEOUT 120)
    endforeach()
endif()
