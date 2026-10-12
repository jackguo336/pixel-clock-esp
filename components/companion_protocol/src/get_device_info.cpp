#include "companion_protocol/get_device_info.hpp"

#include "byte_cursor.hpp"
#include "write_frame_header.hpp"

namespace companion_protocol {

static_assert(kHeaderBytes <= kMaxFrameBytes);

std::size_t encode_get_device_info(uint16_t request_id, std::span<uint8_t> out)
{
    constexpr std::size_t kFrameBytes = kHeaderBytes;
    if (out.size() < kFrameBytes) {
        return 0;
    }
    ByteWriter writer(out);
    if (!write_frame_header(writer, MessageType::GetDeviceInfo, request_id, 0) || writer.written() != kFrameBytes) {
        return 0;
    }
    return kFrameBytes;
}

Status decode_get_device_info(std::span<const uint8_t> body)
{
    ByteReader reader(body);
    if (!reader.empty()) {
        return Status::MalformedBody;
    }
    return Status::Ok;
}

}  // namespace companion_protocol
