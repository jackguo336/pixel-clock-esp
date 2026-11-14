#pragma once

#include <cstdint>
#include <span>

#include "ble_link/message_transport.hpp"

namespace ble_link {

/**
 * The clock's Bluetooth LE peripheral for the companion app.
 * Use once per boot to make the clock visible, accept one phone, and pair it
 * with Just Works. Connect and disconnect reach the runtime as
 * CompanionConnected and CompanionDisconnected.
 */
class BleMessageTransport : public MessageTransport {
public:
    BleMessageTransport() = default;

    /// Runs once per boot; a second call returns false. A failed start leaves
    /// the radio off and the rest of the firmware running.
    [[nodiscard]] bool start();

    /// Always false; the message data path is not implemented. The bytes are not read.
    [[nodiscard]] bool send(std::span<const uint8_t> message) override;
};

}  // namespace ble_link
