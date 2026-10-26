# Source/Project64-test: core tests, run by ctest.

add_executable(Project64-test
    "${PJ64_SRC}/Project64-test/main.cpp"
    "${PJ64_SRC}/Project64-test/GameKeys.cpp")
pj64_target(Project64-test Project64-test)
target_link_libraries(Project64-test PRIVATE Project64-core)

add_test(NAME game-keys COMMAND Project64-test game-keys "${PROJECT_BINARY_DIR}/test/game-keys")
add_test(NAME base-dir COMMAND Project64-test base-dir "${PROJECT_BINARY_DIR}/test/base-dir")
