# GoogleTest target for the companion session. Added by the shared runner at
# test/native_host/; does not load ESP-IDF. Compiles the session against host
# stubs and the companion_protocol codec. The NVS store stays on the Unity target.
set(BLE_LINK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(COMPANION_PROTOCOL_ROOT "${BLE_LINK_ROOT}/../companion_protocol")
set(PLATFORM_ROOT "${BLE_LINK_ROOT}/../platform")
set(SETTINGS_ROOT "${BLE_LINK_ROOT}/../settings")

add_executable(ble_link
    test_inbound_message_pool.cpp
    test_companion_inbound_message_consumer.cpp
    test_inbound_message_router.cpp
    test_get_device_info_handler.cpp
    test_set_wifi_credentials_handler.cpp
    stubs/esp_err.cpp
    "${BLE_LINK_ROOT}/src/inbound_message_pool.cpp"
    "${BLE_LINK_ROOT}/src/companion_inbound_message_consumer.cpp"
    "${BLE_LINK_ROOT}/src/companion_message_reply_sender.cpp"
    "${BLE_LINK_ROOT}/src/get_device_info_handler.cpp"
    "${BLE_LINK_ROOT}/src/set_wifi_credentials_handler.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/message_encoder.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/message_decoder.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/get_device_info.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/set_wifi_credentials.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/ack.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/device_info.cpp"
    "${COMPANION_PROTOCOL_ROOT}/src/error.cpp"
)

target_include_directories(ble_link
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/stubs"
        "${BLE_LINK_ROOT}/include"
        "${COMPANION_PROTOCOL_ROOT}/include"
        "${COMPANION_PROTOCOL_ROOT}/src"
        "${PLATFORM_ROOT}/include"
        "${SETTINGS_ROOT}/include"
)

target_compile_options(ble_link
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(ble_link
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ble_link TEST_PREFIX "ble_link.")
