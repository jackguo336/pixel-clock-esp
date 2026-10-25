# GoogleTest target for the companion_protocol component. Added by the shared
# runner at test/native_host/; does not load ESP-IDF. Compiles host-safe
# production sources directly and exposes both public and private include paths.
set(COMPANION_PROTOCOL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

add_executable(companion_protocol
    test_message_encoder.cpp
    test_message_decoder.cpp
    test_get_device_info.cpp
    test_set_wifi_credentials.cpp
    test_ack.cpp
    test_device_info.cpp
    test_error.cpp
    test_bounded_bytes.cpp
    "${COMPANION_PROTOCOL_ROOT}/src/message_encoder.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/message_decoder.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/get_device_info.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/set_wifi_credentials.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/ack.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/device_info.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/error.cpp"
)

target_include_directories(companion_protocol
    PRIVATE
        "${COMPANION_PROTOCOL_ROOT}/include"
        "${COMPANION_PROTOCOL_ROOT}/src"
)

target_compile_options(companion_protocol
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(companion_protocol
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(companion_protocol TEST_PREFIX "companion_protocol.")
