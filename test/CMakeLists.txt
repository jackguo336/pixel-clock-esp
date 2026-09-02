# Test project: builds component tests under components/*/test/.
# The firmware app in the parent directory does not compile those tests.
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS "../components")
set(TEST_COMPONENTS "platform" CACHE STRING "List of components to test")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(pixel_clock_esp_test)
