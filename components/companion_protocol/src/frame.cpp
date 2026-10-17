#include "companion_protocol/frame.hpp"

#include "byte_cursor.hpp"

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

}  // namespace

Status decode_header(std::span<const uint8_t> bytes, FrameHeader& header)
{
    if (bytes.size() < kHeaderBytes) {
        return Status::FrameTooShort;
    }

    ByteReader reader(bytes);
    uint8_t version = 0;
    uint8_t message_type = 0;
    uint8_t flags = 0;
    uint16_t request_id = 0;
    uint16_t body_length = 0;
    if (!reader.read_u8(version) || !reader.read_u8(message_type) || !reader.read_u8(flags)
        || !reader.read_u16(request_id) || !reader.read_u16(body_length)) {
        return Status::FrameTooShort;
    }

    header.version = version;
    header.message_type = static_cast<MessageType>(message_type);
    header.flags = flags;
    header.request_id = request_id;
    header.body_length = body_length;
    return Status::Ok;
}

Status decode_frame(std::span<const uint8_t> bytes, Frame& frame)
{
    if (bytes.size() < kHeaderBytes) {
        return Status::FrameTooShort;
    }

    FrameHeader header{};
    const Status header_status = decode_header(bytes, header);
    if (header_status != Status::Ok) {
        return header_status;
    }

    frame.header = header;
    frame.body = {};

    if (bytes.size() > kMaxFrameBytes) {
        return Status::FrameTooLong;
    }
    if (header.version != kProtocolVersion) {
        return Status::UnsupportedVersion;
    }
    if (header.flags != kFlagsNone) {
        return Status::UnsupportedFlags;
    }

    const std::size_t actual_body_length = bytes.size() - kHeaderBytes;
    if (header.body_length != actual_body_length) {
        return Status::BodyLengthMismatch;
    }
    if (!is_known_message_type(header.message_type)) {
        return Status::UnknownMessageType;
    }

    frame.body = std::span<const uint8_t>(bytes.data() + kHeaderBytes, actual_body_length);
    return Status::Ok;
}

}  // namespace companion_protocol
