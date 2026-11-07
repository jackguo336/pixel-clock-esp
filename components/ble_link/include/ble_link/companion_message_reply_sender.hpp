#pragma once

#include <cstdint>

#include "ble_link/message_transport.hpp"
#include "companion_protocol/message.hpp"

namespace ble_link {

class CompanionMessageReplySender {
public:
    explicit CompanionMessageReplySender(MessageTransport& transport);

    void send_reply(uint16_t request_id, const companion_protocol::MessageBody& body);

private:
    MessageTransport& transport_;
};

}  // namespace ble_link
