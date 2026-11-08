#pragma once

#include "companion_protocol/message.hpp"

namespace ble_link {

/**
 * Handles one decoded companion request.
 * Sending a reply is optional when the handler owns the request.
 */
class InboundMessageHandler {
public:
    virtual ~InboundMessageHandler() = default;

    InboundMessageHandler(const InboundMessageHandler&) = delete;
    InboundMessageHandler& operator=(const InboundMessageHandler&) = delete;

    /// Returns true when this handler owns the request.
    /// Returns false when it does not. Sending a reply is optional.
    [[nodiscard]] virtual bool try_handle(const companion_protocol::Message& request) = 0;

protected:
    InboundMessageHandler() = default;
};

}  // namespace ble_link
