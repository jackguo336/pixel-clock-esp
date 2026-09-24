# GoogleTest target for the led_matrix mapping. Added by the shared runner at
# test/native_host/; does not load ESP-IDF. Compiles only the host-safe mapper.
set(LED_MATRIX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(DISPLAY_INCLUDE "${LED_MATRIX_ROOT}/../display/include")

add_executable(led_matrix
    test_led_matrix_map.cpp
    "${LED_MATRIX_ROOT}/src/led_matrix_map.cpp"
)

target_include_directories(led_matrix
    PRIVATE
        "${LED_MATRIX_ROOT}/include"
        "${LED_MATRIX_ROOT}/src"
        "${DISPLAY_INCLUDE}"
)

target_compile_options(led_matrix
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(led_matrix
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(led_matrix TEST_PREFIX "led_matrix.")
