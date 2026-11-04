#pragma once

#include <span>

#include "ble_link/inbound_message_pool.hpp"
#include "ble_link/inbound_message_router.hpp"
#include "ble_link/message_transport.hpp"
#include "platform/runtime_component.hpp"

namespace ble_link {

/**
 * Turns companion connection and message events into handler replies.
 * Use on the control task. The task that posts fills the pool.
 */
class CompanionInboundMessageConsumer : public platform::RuntimeComponent {
public:
    CompanionInboundMessageConsumer(InboundMessagePool& inbound_messages, InboundMessageRouter& router,
                                    MessageTransport& transport);

    CompanionInboundMessageConsumer(const CompanionInboundMessageConsumer&) = delete;
    CompanionInboundMessageConsumer& operator=(const CompanionInboundMessageConsumer&) = delete;

    const char* name() const override;
    void on_event(const platform::Event& event) override;

    /// Decodes one complete message. A failed decode sends Error.
    /// A successful decode is passed to the router.
    void consume(std::span<const uint8_t> message);

private:
    InboundMessagePool& inbound_messages_;
    InboundMessageRouter& router_;
    MessageTransport& transport_;
    bool companion_connected_{false};
};

}  // namespace ble_link
