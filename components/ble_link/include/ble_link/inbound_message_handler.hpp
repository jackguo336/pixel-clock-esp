#pragma once

#include <cstdint>

#include "ble_link/message_transport.hpp"
#include "companion_protocol/message.hpp"

namespace ble_link {

void send_reply(MessageTransport& transport, uint16_t request_id, const companion_protocol::MessageBody& body);

/**
 * Reply for one decoded companion request.
 * Use when a request type encodes and sends its own response.
 */
class InboundMessageHandler {
public:
    virtual ~InboundMessageHandler() = default;

    InboundMessageHandler(const InboundMessageHandler&) = delete;
    InboundMessageHandler& operator=(const InboundMessageHandler&) = delete;

    /// Encodes and sends when this handler owns the decoded request.
    /// Returns false without sending when it does not.
    [[nodiscard]] virtual bool try_handle(const companion_protocol::Message& request) = 0;

protected:
    InboundMessageHandler() = default;
};

}  // namespace ble_link
