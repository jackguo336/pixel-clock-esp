# GoogleTest target for the file_system component. Added by the shared runner at
# test/native_host/; does not load ESP-IDF. Compile only host-safe sources
# (typically public headers under include/).
add_executable(file_system
    test_managed_file_descriptor.cpp
)

target_include_directories(file_system
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

target_compile_options(file_system
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(file_system
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(file_system TEST_PREFIX "file_system.")
