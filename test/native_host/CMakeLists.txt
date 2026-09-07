# Native-host GoogleTest project. Compiles for macOS/Linux and does not load
# ESP-IDF. Add only device-independent sources and public component headers.
cmake_minimum_required(VERSION 3.22)

project(pixel_clock_esp_native_host_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Pin GoogleTest so host CI and local builds share the same release.
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(NATIVE_HOST_PLATFORM_INCLUDE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../components/platform/include")

add_executable(native_host_tests
    test_message.cpp
)

target_include_directories(native_host_tests
    PRIVATE
        "${NATIVE_HOST_PLATFORM_INCLUDE}"
)

target_compile_options(native_host_tests
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(native_host_tests
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(native_host_tests)
