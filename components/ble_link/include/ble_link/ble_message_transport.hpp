#pragma once

#include <cstdint>
#include <span>

#include "ble_link/inbound_message_pool.hpp"
#include "ble_link/message_transport.hpp"

namespace ble_link {

/**
 * Lets the pixel clock connect with a companion phone over Bluetooth and
 * receive and reply to messages.
 */
class BleMessageTransport : public MessageTransport {
public:
    explicit BleMessageTransport(InboundMessagePool& inbound_messages);

    /// Runs once per boot; a second call returns false. A failed start leaves
    /// the radio off and the rest of the firmware running.
    [[nodiscard]] bool start();

    /// Sends the encoded message. The bytes are valid only for this call.
    [[nodiscard]] bool send(std::span<const uint8_t> message) override;

private:
    InboundMessagePool& inbound_messages_;
};

}  // namespace ble_link
