#include "companion_protocol/message_encoder.hpp"

#include <optional>
#include <type_traits>

#include "byte_writer.hpp"

namespace companion_protocol {
namespace {

struct MessageTypeAndEncodedSize {
    MessageType message_type{};
    std::size_t encoded_size{0};
};

[[nodiscard]] std::optional<MessageTypeAndEncodedSize>
message_type_and_encoded_size(const MessageBody& message_body)
{
    return std::visit(
        [](const auto& body) -> std::optional<MessageTypeAndEncodedSize> {
            using Body = std::decay_t<decltype(body)>;
            if constexpr (std::is_same_v<Body, std::monostate>) {
                return std::nullopt;
            } else {
                const std::optional<std::size_t> encoded_size = body.encoded_body_size();
                if (!encoded_size.has_value()) {
                    return std::nullopt;
                }
                return MessageTypeAndEncodedSize{Body::kMessageType, *encoded_size};
            }
        },
        message_body);
}

[[nodiscard]] bool write_message_header(ByteWriter& writer, MessageType type, uint16_t request_id,
                                      uint16_t body_length)
{
    return writer.write_u8(kProtocolVersion) && writer.write_u8(static_cast<uint8_t>(type))
        && writer.write_u8(kFlagsNone) && writer.write_u16(request_id) && writer.write_u16(body_length);
}

[[nodiscard]] bool write_message_body(const MessageBody& message_body, std::span<uint8_t> out)
{
    return std::visit(
        [&](const auto& body) -> bool {
            using Body = std::decay_t<decltype(body)>;
            if constexpr (std::is_same_v<Body, std::monostate>) {
                return false;
            } else {
                return body.encode(out);
            }
        },
        message_body);
}

}  // namespace

std::size_t encode_message(const Message& message, std::span<uint8_t> out)
{
    const std::optional<MessageTypeAndEncodedSize> body_type_and_size =
        message_type_and_encoded_size(message.body);
    if (!body_type_and_size.has_value()) {
        return 0;
    }

    const std::size_t message_length = kHeaderBytes + body_type_and_size->encoded_size;
    if (out.size() < message_length) {
        return 0;
    }

    ByteWriter writer(out);
    if (!write_message_header(writer, body_type_and_size->message_type, message.header.request_id,
                            static_cast<uint16_t>(body_type_and_size->encoded_size))) {
        return 0;
    }

    const bool wrote_body = write_message_body(
        message.body, out.subspan(kHeaderBytes, body_type_and_size->encoded_size));
    if (!wrote_body || writer.written() != kHeaderBytes) {
        return 0;
    }
    return message_length;
}

}  // namespace companion_protocol
