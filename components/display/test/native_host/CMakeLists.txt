# GoogleTest targets for led_matrix. Added by the shared runner at
# test/native_host/; does not load ESP-IDF. The mapping suite compiles the
# real mapper. The output suite compiles led_matrix_output.cpp against host
# stubs and a test double for led_index_for_logical_pixel.
set(LED_MATRIX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(GRAPHICS_ROOT "${LED_MATRIX_ROOT}/../graphics")
set(GRAPHICS_INCLUDE "${GRAPHICS_ROOT}/include")

add_executable(led_matrix
    test_led_matrix_map.cpp
    "${LED_MATRIX_ROOT}/src/led_matrix_map.cpp"
)

target_include_directories(led_matrix
    PRIVATE
        "${LED_MATRIX_ROOT}/include"
        "${LED_MATRIX_ROOT}/src"
        "${GRAPHICS_INCLUDE}"
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

add_executable(led_matrix_output
    test_led_matrix_output.cpp
    mock_led_strip.cpp
    mock_led_index.cpp
    stubs/esp_err.cpp
    "${LED_MATRIX_ROOT}/src/led_matrix_output.cpp"
    "${GRAPHICS_ROOT}/src/logical_framebuffer.cpp"
)

target_include_directories(led_matrix_output
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/stubs"
        "${LED_MATRIX_ROOT}/include"
        "${LED_MATRIX_ROOT}/src"
        "${GRAPHICS_INCLUDE}"
)

target_compile_options(led_matrix_output
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(led_matrix_output
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(led_matrix TEST_PREFIX "led_matrix.")
gtest_discover_tests(led_matrix_output TEST_PREFIX "led_matrix.")
