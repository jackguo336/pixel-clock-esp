#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "ble_link/message_transport.hpp"

class FakeMessageTransport : public ble_link::MessageTransport {
public:
    explicit FakeMessageTransport(bool send_result = true)
        : send_result_(send_result)
    {
    }

    bool send(std::span<const uint8_t> message) override
    {
        ++send_count;
        sent.assign(message.begin(), message.end());
        return send_result_;
    }

    int send_count{0};
    std::vector<uint8_t> sent;

private:
    bool send_result_;
};
