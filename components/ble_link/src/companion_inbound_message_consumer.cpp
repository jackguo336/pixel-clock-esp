#include "ble_link/companion_inbound_message_consumer.hpp"

#include <array>
#include <cstring>

#include "companion_protocol/error.hpp"
#include "companion_protocol/message_decoder.hpp"
#include "platform/log.hpp"

namespace ble_link {
namespace {

// The error reply includes the request id when the request can be partly decoded, and uses request id 0 otherwise.
void send_decode_error(CompanionMessageReplySender& reply_sender, std::span<const uint8_t> message,
                       companion_protocol::Status status, const companion_protocol::Message& decoded)
{
    uint16_t request_id = 0;
    if (status == companion_protocol::Status::MessageTooLong) {
        companion_protocol::MessageHeader header{};
        if (companion_protocol::decode_header(message, header) == companion_protocol::Status::Ok) {
            request_id = header.request_id;
        }
    } else if (status != companion_protocol::Status::MessageTooShort) {
        request_id = decoded.header.request_id;
    }

    reply_sender.send_reply(request_id, companion_protocol::MessageBody{companion_protocol::Error{.code = status}});
}

}  // namespace

CompanionInboundMessageConsumer::CompanionInboundMessageConsumer(InboundMessagePool& inbound_messages,
                                                                 std::span<InboundMessageHandler* const> handlers,
                                                                 CompanionMessageReplySender& reply_sender)
    : inbound_messages_(inbound_messages)
    , handlers_(handlers)
    , reply_sender_(reply_sender)
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

    // A payload of any other size is not a message ref. payload_size 0 is not slot 0.
    if (event.payload_size != sizeof(InboundMessagePoolSlotRef)) {
        PLATFORM_LOGW(this, "message event payload is %u bytes", static_cast<unsigned>(event.payload_size));
        return;
    }

    InboundMessagePoolSlotRef message_ref{};
    std::memcpy(&message_ref, event.payload, sizeof(message_ref));

    std::array<uint8_t, companion_protocol::kMaxMessageBytes> message_bytes{};
    const std::size_t copied_length = inbound_messages_.read_then_free_slot(message_ref.slot_index, message_bytes);
    if (copied_length != message_ref.message_length) {
        PLATFORM_LOGW(this, "slot %u copied %u expected %u", static_cast<unsigned>(message_ref.slot_index),
                      static_cast<unsigned>(copied_length), static_cast<unsigned>(message_ref.message_length));
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
        send_decode_error(reply_sender_, message, status, decoded);
        return;
    }

    for (InboundMessageHandler* const handler : handlers_) {
        if (handler->try_handle(decoded)) {
            return;
        }
    }
    PLATFORM_LOGW(this, "no handler for message type %u", static_cast<unsigned>(decoded.header.message_type));
}

}  // namespace ble_link
