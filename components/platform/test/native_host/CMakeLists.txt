# GoogleTest target for the platform component. Added by the shared runner at
# test/native_host/; does not load ESP-IDF. Compile only host-safe sources
# (typically public headers under include/).
add_executable(platform
    test_message.cpp
)

target_include_directories(platform
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

target_compile_options(platform
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(platform
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(platform TEST_PREFIX "platform.")
