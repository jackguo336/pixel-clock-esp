#include "companion_protocol/device_info.hpp"

#include <array>

#include "byte_cursor.hpp"
#include "write_frame_header.hpp"

namespace companion_protocol {
namespace {

constexpr std::size_t kDeviceInfoMaxFrameBytes = kHeaderBytes + 1 + 1 + kMaxFirmwareVersionBytes;

static_assert(kDeviceInfoMaxFrameBytes <= kMaxFrameBytes);

}  // namespace

std::size_t encode_device_info(uint16_t request_id, const DeviceInfo& info, std::span<uint8_t> out)
{
    const std::size_t version_size = info.firmware_version.size();
    if (version_size > kMaxFirmwareVersionBytes) {
        return 0;
    }

    const std::size_t body_length = 2 + version_size;
    const std::size_t frame_length = kHeaderBytes + body_length;
    if (out.size() < frame_length) {
        return 0;
    }

    ByteWriter writer(out);
    if (!write_frame_header(writer, MessageType::DeviceInfo, request_id, static_cast<uint16_t>(body_length))
        || !writer.write_u8(info.protocol_version) || !writer.write_u8(static_cast<uint8_t>(version_size))
        || !writer.write_bytes(info.firmware_version.view()) || writer.written() != frame_length) {
        return 0;
    }
    return frame_length;
}

Status decode_device_info(std::span<const uint8_t> body, DeviceInfo& info)
{
    ByteReader reader(body);
    uint8_t protocol_version = 0;
    uint8_t version_length = 0;
    if (!reader.read_u8(protocol_version) || !reader.read_u8(version_length)) {
        return Status::MalformedBody;
    }
    if (version_length > kMaxFirmwareVersionBytes) {
        return Status::InvalidField;
    }

    std::array<uint8_t, kMaxFirmwareVersionBytes> version_bytes{};
    if (!reader.read_bytes(std::span<uint8_t>(version_bytes.data(), version_length)) || !reader.empty()) {
        return Status::MalformedBody;
    }

    DeviceInfo decoded{};
    decoded.protocol_version = protocol_version;
    if (!decoded.firmware_version.assign(std::span<const uint8_t>(version_bytes.data(), version_length))) {
        return Status::InvalidField;
    }
    info = decoded;
    return Status::Ok;
}

}  // namespace companion_protocol
