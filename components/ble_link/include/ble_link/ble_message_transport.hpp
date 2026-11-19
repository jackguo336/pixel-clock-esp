#pragma once

#include <cstdint>
#include <span>

#include "ble_link/inbound_message_pool.hpp"
#include "ble_link/message_transport.hpp"

namespace ble_link {

/**
 * Lets the pixel clock connect with a companion phone over Bluetooth and receive and reply to messages.
 */
class BleMessageTransport : public MessageTransport {
public:
    explicit BleMessageTransport(InboundMessagePool& inbound_messages);

    /// Runs once per boot; a second call returns false. A failed start leaves
    /// the radio off and the rest of the firmware running.
    [[nodiscard]] bool start();

    /// Notifies the encoded message as pieces. The bytes are valid only for this call.
    /// Returns false when there is no connection, notifications are off, the message
    /// is empty or longer than 128 bytes, or a notification fails.
    [[nodiscard]] bool send(std::span<const uint8_t> message) override;

private:
    InboundMessagePool& inbound_messages_;
};

}  // namespace ble_link
