#include "ble_link/companion_inbound_message_consumer.hpp"

#include <array>
#include <cstring>

#include "ble_link/inbound_message_handler.hpp"
#include "companion_protocol/error.hpp"
#include "companion_protocol/message_decoder.hpp"
#include "platform/log.hpp"

namespace ble_link {
namespace {

// MessageTooShort leaves the decoded message unchanged, so its request id is not available.
// MessageTooLong does too; the id is in the leading header when those bytes decode.
[[nodiscard]] uint16_t request_id_for_decode_failure(std::span<const uint8_t> message,
                                                     companion_protocol::Status status,
                                                     const companion_protocol::Message& decoded)
{
    if (status == companion_protocol::Status::MessageTooShort) {
        return 0;
    }
    if (status == companion_protocol::Status::MessageTooLong) {
        companion_protocol::MessageHeader header{};
        if (companion_protocol::decode_header(message, header) != companion_protocol::Status::Ok) {
            return 0;
        }
        return header.request_id;
    }
    return decoded.header.request_id;
}

}  // namespace

CompanionInboundMessageConsumer::CompanionInboundMessageConsumer(InboundMessagePool& inbound_messages,
                                                                 InboundMessageRouter& router,
                                                                 MessageTransport& transport)
    : inbound_messages_(inbound_messages)
    , router_(router)
    , transport_(transport)
{
}

const char* CompanionInboundMessageConsumer::name() const
{
    return "companion_inbound_message_consumer";
}

void CompanionInboundMessageConsumer::on_event(const platform::Event& event)
{
    if (event.type == platform::EventType::CompanionConnected) {
        companion_connected_ = true;
        return;
    }
    if (event.type == platform::EventType::CompanionDisconnected) {
        companion_connected_ = false;
        return;
    }
    if (event.type != platform::EventType::CompanionMessageReceived) {
        return;
    }

    // A payload of any other size is not a handoff. payload_size 0 is not entry 0.
    if (event.payload_size != sizeof(InboundMessagePoolEntryRef)) {
        PLATFORM_LOGW(this, "message event payload is %u bytes", static_cast<unsigned>(event.payload_size));
        return;
    }

    InboundMessagePoolEntryRef handoff{};
    std::memcpy(&handoff, event.payload, sizeof(handoff));

    std::array<uint8_t, companion_protocol::kMaxMessageBytes> message_bytes{};
    const std::size_t copied_length = inbound_messages_.read_then_remove(handoff.entry_index, message_bytes);
    if (copied_length != handoff.message_length) {
        PLATFORM_LOGW(this, "entry %u copied %u expected %u", static_cast<unsigned>(handoff.entry_index),
                      static_cast<unsigned>(copied_length), static_cast<unsigned>(handoff.message_length));
        return;
    }
    if (!companion_connected_) {
        return;
    }

    consume(std::span<const uint8_t>(message_bytes.data(), copied_length));
}

void CompanionInboundMessageConsumer::consume(std::span<const uint8_t> message)
{
    companion_protocol::Message decoded{};
    const companion_protocol::Status status = companion_protocol::decode_message(message, decoded);
    if (status != companion_protocol::Status::Ok) {
        PLATFORM_LOGE(this, "decode failed status=%u", static_cast<unsigned>(status));
        const uint16_t request_id = request_id_for_decode_failure(message, status, decoded);
        send_reply(transport_, request_id,
                   companion_protocol::MessageBody{companion_protocol::Error{.code = status}});
        return;
    }

    if (!router_.route(decoded)) {
        PLATFORM_LOGW(this, "no handler for message type %u",
                      static_cast<unsigned>(decoded.header.message_type));
    }
}

}  // namespace ble_link
