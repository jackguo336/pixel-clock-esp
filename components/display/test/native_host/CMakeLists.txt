# GoogleTest target for the display component. Added by the shared runner at
# test/native_host/; does not load ESP-IDF. Compiles host-safe production
# sources directly and exposes both public and private include paths.
set(DISPLAY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

add_executable(display
    test_bitmap_views.cpp
    test_logical_framebuffer.cpp
    test_bitmap_file_loader.cpp
    test_bitmap_rasterizer.cpp
    "${DISPLAY_ROOT}/src/bitmap.cpp"
    "${DISPLAY_ROOT}/src/logical_framebuffer.cpp"
    "${DISPLAY_ROOT}/src/bitmap_file_loader.cpp"
    "${DISPLAY_ROOT}/src/bitmap_rasterizer.cpp"
)

target_include_directories(display
    PRIVATE
        "${DISPLAY_ROOT}/include"
        "${DISPLAY_ROOT}/src"
)

target_compile_options(display
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(display
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(display TEST_PREFIX "display.")
