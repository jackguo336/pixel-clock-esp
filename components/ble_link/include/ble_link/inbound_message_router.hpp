#pragma once

#include <span>

#include "ble_link/inbound_message_handler.hpp"

namespace ble_link {

/**
 * Chooses the handler that owns a decoded companion request.
 * Use when several request types share one inbound path.
 */
class InboundMessageRouter {
public:
    explicit InboundMessageRouter(std::span<InboundMessageHandler* const> handlers);

    /// Calls the first handler that owns request. Does not decode, encode, or send.
    [[nodiscard]] bool route(const companion_protocol::Message& request);

private:
    std::span<InboundMessageHandler* const> handlers_;
};

}  // namespace ble_link
