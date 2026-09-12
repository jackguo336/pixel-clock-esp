# Shared native-host GoogleTest/CTest runner. Fetches GoogleTest, then adds
# each participating component's native-host CMake project. Does not load
# ESP-IDF. Component suites live under components/<name>/test/native_host/.
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

# Components that own a native-host suite. Each must provide
# components/<name>/test/native_host/CMakeLists.txt.
set(NATIVE_HOST_TEST_COMPONENTS
    platform
    display
)

foreach(component IN LISTS NATIVE_HOST_TEST_COMPONENTS)
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/../../components/${component}/test/native_host"
        "${CMAKE_CURRENT_BINARY_DIR}/${component}"
    )
endforeach()
