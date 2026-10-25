#include "companion_protocol/message_decoder.hpp"

#include <utility>

#include "byte_reader.hpp"

namespace companion_protocol {
namespace {

[[nodiscard]] bool is_known_message_type(MessageType type)
{
    switch (static_cast<uint8_t>(type)) {
    case static_cast<uint8_t>(MessageType::GetDeviceInfo):
    case static_cast<uint8_t>(MessageType::SetWifiCredentials):
    case static_cast<uint8_t>(MessageType::Ack):
    case static_cast<uint8_t>(MessageType::DeviceInfo):
    case static_cast<uint8_t>(MessageType::Error):
        return true;
    default:
        return false;
    }
}

[[nodiscard]] Status validate_header(std::span<const uint8_t> bytes, const MessageHeader& header)
{
    if (header.version != kProtocolVersion) {
        return Status::UnsupportedVersion;
    }
    if (header.flags != kFlagsNone) {
        return Status::UnsupportedFlags;
    }
    if (header.body_length != bytes.size() - kHeaderBytes) {
        return Status::BodyLengthMismatch;
    }
    if (!is_known_message_type(header.message_type)) {
        return Status::UnknownMessageType;
    }
    return Status::Ok;
}

template <typename Body>
[[nodiscard]] Status decode_message_body(std::span<const uint8_t> bytes, MessageBody& decoded_body)
{
    Body body{};
    const Status status = body.decode(bytes);
    if (status != Status::Ok) {
        return status;
    }
    decoded_body = body;
    return Status::Ok;
}

[[nodiscard]] Status decode_message_body(MessageType message_type, std::span<const uint8_t> bytes,
                                         MessageBody& decoded_body)
{
    switch (message_type) {
    case MessageType::GetDeviceInfo:
        return decode_message_body<GetDeviceInfo>(bytes, decoded_body);
    case MessageType::SetWifiCredentials:
        return decode_message_body<SetWifiCredentials>(bytes, decoded_body);
    case MessageType::Ack:
        return decode_message_body<Ack>(bytes, decoded_body);
    case MessageType::DeviceInfo:
        return decode_message_body<DeviceInfo>(bytes, decoded_body);
    case MessageType::Error:
        return decode_message_body<Error>(bytes, decoded_body);
    }
    return Status::UnknownMessageType;
}

}  // namespace

Status decode_header(std::span<const uint8_t> bytes, MessageHeader& header)
{
    if (bytes.size() < kHeaderBytes) {
        return Status::MessageTooShort;
    }

    ByteReader reader(bytes);
    uint8_t version = 0;
    uint8_t message_type = 0;
    uint8_t flags = 0;
    uint16_t request_id = 0;
    uint16_t body_length = 0;
    if (!reader.read_u8(version) || !reader.read_u8(message_type) || !reader.read_u8(flags)
        || !reader.read_u16(request_id) || !reader.read_u16(body_length)) {
        return Status::MessageTooShort;
    }

    header.version = version;
    header.message_type = static_cast<MessageType>(message_type);
    header.flags = flags;
    header.request_id = request_id;
    header.body_length = body_length;
    return Status::Ok;
}

Status decode_message(std::span<const uint8_t> bytes, Message& message)
{
    if (bytes.size() > kMaxMessageBytes) {
        return Status::MessageTooLong;
    }

    MessageHeader header{};
    const Status header_status = decode_header(bytes, header);
    if (header_status != Status::Ok) {
        return header_status;
    }

    const auto reject = [&](Status status) {
        message.header = header;
        message.body = std::monostate{};
        return status;
    };

    const Status validation_status = validate_header(bytes, header);
    if (validation_status != Status::Ok) {
        return reject(validation_status);
    }

    const std::size_t actual_body_length = bytes.size() - kHeaderBytes;
    const std::span<const uint8_t> body_bytes(bytes.data() + kHeaderBytes, actual_body_length);
    MessageBody decoded_body{};
    const Status body_status = decode_message_body(header.message_type, body_bytes, decoded_body);
    if (body_status != Status::Ok) {
        return reject(body_status);
    }

    message.header = header;
    message.body = std::move(decoded_body);
    return Status::Ok;
}

}  // namespace companion_protocol
