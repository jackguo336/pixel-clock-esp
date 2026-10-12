#include "companion_protocol/error.hpp"

#include "byte_cursor.hpp"
#include "write_frame_header.hpp"

namespace companion_protocol {

std::size_t encode_error(uint16_t request_id, const Error& error, std::span<uint8_t> out)
{
    if (out.size() < kErrorFrameBytes) {
        return 0;
    }
    ByteWriter writer(out);
    if (!write_frame_header(writer, MessageType::Error, request_id, static_cast<uint16_t>(kErrorBodyBytes))
        || !writer.write_u8(static_cast<uint8_t>(error.code))
        || !writer.write_u8(static_cast<uint8_t>(error.failed_message_type)) || writer.written() != kErrorFrameBytes) {
        return 0;
    }
    return kErrorFrameBytes;
}

Status decode_error(std::span<const uint8_t> body, Error& error)
{
    ByteReader reader(body);
    uint8_t code = 0;
    uint8_t failed_message_type = 0;
    if (!reader.read_u8(code) || !reader.read_u8(failed_message_type) || !reader.empty()) {
        return Status::MalformedBody;
    }
    error.code = static_cast<Status>(code);
    error.failed_message_type = static_cast<MessageType>(failed_message_type);
    return Status::Ok;
}

}  // namespace companion_protocol
