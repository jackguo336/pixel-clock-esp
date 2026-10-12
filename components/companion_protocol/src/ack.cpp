#include "companion_protocol/ack.hpp"

#include "byte_cursor.hpp"
#include "write_frame_header.hpp"

namespace companion_protocol {

std::size_t encode_ack(uint16_t request_id, const Ack& ack, std::span<uint8_t> out)
{
    if (out.size() < kAckFrameBytes) {
        return 0;
    }
    ByteWriter writer(out);
    if (!write_frame_header(writer, MessageType::Ack, request_id, static_cast<uint16_t>(kAckBodyBytes))
        || !writer.write_u8(static_cast<uint8_t>(ack.status)) || writer.written() != kAckFrameBytes) {
        return 0;
    }
    return kAckFrameBytes;
}

Status decode_ack(std::span<const uint8_t> body, Ack& ack)
{
    ByteReader reader(body);
    uint8_t status = 0;
    if (!reader.read_u8(status) || !reader.empty()) {
        return Status::MalformedBody;
    }
    ack.status = static_cast<Status>(status);
    return Status::Ok;
}

}  // namespace companion_protocol
