#include "companion_protocol/device_info.hpp"

#include <array>

#include "byte_reader.hpp"
#include "byte_writer.hpp"

namespace companion_protocol {

std::optional<std::size_t> DeviceInfo::encoded_body_size() const
{
    const std::size_t version_size = firmware_version.size();
    if (version_size > kMaxFirmwareVersionBytes) {
        return std::nullopt;
    }
    return kProtocolVersionFieldBytes + kFirmwareVersionLengthPrefixBytes + version_size;
}

bool DeviceInfo::encode(std::span<uint8_t> out) const
{
    const std::optional<std::size_t> body_size = encoded_body_size();
    if (!body_size.has_value() || out.size() < *body_size) {
        return false;
    }

    const std::size_t version_size = firmware_version.size();
    ByteWriter writer(out);
    return writer.write_u8(protocol_version) && writer.write_u8(static_cast<uint8_t>(version_size))
        && writer.write_bytes(firmware_version.view()) && writer.written() == *body_size;
}

Status DeviceInfo::decode(std::span<const uint8_t> bytes)
{
    ByteReader reader(bytes);
    uint8_t protocol_version_byte = 0;
    uint8_t version_length = 0;
    if (!reader.read_u8(protocol_version_byte) || !reader.read_u8(version_length)) {
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
    decoded.protocol_version = protocol_version_byte;
    if (!decoded.firmware_version.assign(std::span<const uint8_t>(version_bytes.data(), version_length))) {
        return Status::InvalidField;
    }
    *this = decoded;
    return Status::Ok;
}

}  // namespace companion_protocol
