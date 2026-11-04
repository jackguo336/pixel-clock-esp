#include "ble_link/inbound_message_router.hpp"

namespace ble_link {

InboundMessageRouter::InboundMessageRouter(std::span<InboundMessageHandler* const> handlers)
    : handlers_(handlers)
{
}

bool InboundMessageRouter::route(const companion_protocol::Message& request)
{
    for (InboundMessageHandler* const handler : handlers_) {
        if (handler->try_handle(request)) {
            return true;
        }
    }
    return false;
}

}  // namespace ble_link
