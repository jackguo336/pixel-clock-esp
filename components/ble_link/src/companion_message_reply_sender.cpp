#include "ble_link/companion_message_reply_sender.hpp"

#include <array>
#include <span>

#include "companion_protocol/message_encoder.hpp"
#include "esp_log.h"

namespace ble_link {
namespace {

constexpr char kTag[] = "companion_message_reply_sender";

}  // namespace

CompanionMessageReplySender::CompanionMessageReplySender(MessageTransport& transport)
    : transport_(transport)
{
}

void CompanionMessageReplySender::send_reply(uint16_t request_id, const companion_protocol::MessageBody& body)
{
    companion_protocol::Message message{};
    message.header.request_id = request_id;
    message.body = body;

    std::array<uint8_t, companion_protocol::kMaxMessageBytes> encoded{};
    const std::size_t written = companion_protocol::encode_message(message, encoded);
    if (written == 0) {
        ESP_LOGE(kTag, "encode wrote nothing");
        return;
    }

    const bool sent = transport_.send(std::span<const uint8_t>(encoded.data(), written));
    if (!sent) {
        ESP_LOGE(kTag, "send failed");
    }
}

}  // namespace ble_link
