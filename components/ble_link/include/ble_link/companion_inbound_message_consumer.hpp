#pragma once

#include <span>

#include "ble_link/companion_message_reply_sender.hpp"
#include "ble_link/inbound_message_handler.hpp"
#include "ble_link/inbound_message_pool.hpp"
#include "platform/runtime_component.hpp"

namespace ble_link {

/**
 * Takes companion messages from the inbound pool and processes them.
 * A companion message event is the signal to read one message from the pool.
 */
class CompanionInboundMessageConsumer : public platform::RuntimeComponent {
public:
    CompanionInboundMessageConsumer(InboundMessagePool& inbound_messages,
                                    std::span<InboundMessageHandler* const> handlers,
                                    CompanionMessageReplySender& reply_sender);

    CompanionInboundMessageConsumer(const CompanionInboundMessageConsumer&) = delete;
    CompanionInboundMessageConsumer& operator=(const CompanionInboundMessageConsumer&) = delete;

    const char* name() const override;
    void on_event(const platform::Event& event) override;

    void consume(std::span<const uint8_t> message);

private:
    InboundMessagePool& inbound_messages_;
    std::span<InboundMessageHandler* const> handlers_;
    CompanionMessageReplySender& reply_sender_;
    bool companion_connected_{false};
};

}  // namespace ble_link
