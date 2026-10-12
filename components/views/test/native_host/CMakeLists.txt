# GoogleTest target for the views component. Added by the shared runner at
# test/native_host/; does not load ESP-IDF. Compiles the view sources directly,
# plus the graphics sources element-tree layout needs.
set(VIEWS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(GRAPHICS_ROOT "${VIEWS_ROOT}/../graphics")

add_executable(views
    test_views.cpp
    test_carousel_view.cpp
    "${VIEWS_ROOT}/src/carousel_view.cpp"
    "${GRAPHICS_ROOT}/src/bitmap_file.cpp"
    "${GRAPHICS_ROOT}/src/font.cpp"
    "${GRAPHICS_ROOT}/src/text_measurement.cpp"
    "${GRAPHICS_ROOT}/src/element_layout.cpp"
    "${GRAPHICS_ROOT}/src/element_tree.cpp"
)

target_include_directories(views
    PRIVATE
        "${VIEWS_ROOT}/include"
        "${GRAPHICS_ROOT}/include"
        "${GRAPHICS_ROOT}/src"
)

target_compile_options(views
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(views
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(views TEST_PREFIX "views.")
