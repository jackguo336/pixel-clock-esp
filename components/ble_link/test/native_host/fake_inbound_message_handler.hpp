#pragma once

#include "ble_link/inbound_message_handler.hpp"

class FakeInboundMessageHandler : public ble_link::InboundMessageHandler {
public:
    bool try_handle(const companion_protocol::Message& request) override
    {
        ++try_handle_count;
        last_request = request;
        return claims_request;
    }

    bool claims_request{true};
    int try_handle_count{0};
    companion_protocol::Message last_request{};
};
