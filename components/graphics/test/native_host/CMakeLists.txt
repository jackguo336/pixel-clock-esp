# GoogleTest target for the graphics component. Added by the shared runner at
# test/native_host/; does not load ESP-IDF. Compiles host-safe production
# sources directly and exposes both public and private include paths.
set(GRAPHICS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(FILE_SYSTEM_INCLUDE "${GRAPHICS_ROOT}/../file_system/include")

add_executable(graphics
    test_bitmap_file.cpp
    test_logical_framebuffer.cpp
    test_bitmap_file_loader.cpp
    test_bitmap_rasterizer.cpp
    test_element_tree.cpp
    test_element_tree_renderer.cpp
    test_views.cpp
    "${GRAPHICS_ROOT}/src/bitmap_file.cpp"
    "${GRAPHICS_ROOT}/src/logical_framebuffer.cpp"
    "${GRAPHICS_ROOT}/src/bitmap_file_loader.cpp"
    "${GRAPHICS_ROOT}/src/bitmap_rasterizer.cpp"
    "${GRAPHICS_ROOT}/src/element_tree_renderer.cpp"
)

target_include_directories(graphics
    PRIVATE
        "${GRAPHICS_ROOT}/include"
        "${GRAPHICS_ROOT}/src"
        "${FILE_SYSTEM_INCLUDE}"
)

target_compile_options(graphics
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(graphics
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(graphics TEST_PREFIX "graphics.")
