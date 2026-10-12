#include "companion_protocol/messages.hpp"

#include <array>

#include "byte_cursor.hpp"

namespace companion_protocol {
namespace {

constexpr uint8_t kFlagsNone = 0;

// Largest v1 frame: header + ssid_length + 32 SSID bytes + passphrase_length + 64 passphrase bytes.
constexpr std::size_t kSetWifiCredentialsMaxFrameBytes = kHeaderBytes + 1 + kMaxSsidBytes + 1 + kMaxPassphraseBytes;
constexpr std::size_t kDeviceInfoMaxFrameBytes = kHeaderBytes + 1 + 1 + kMaxFirmwareVersionBytes;
constexpr std::size_t kAckFrameBytes = kHeaderBytes + 1;
constexpr std::size_t kErrorFrameBytes = kHeaderBytes + 2;

static_assert(kHeaderBytes <= kMaxFrameBytes);
static_assert(kAckFrameBytes <= kMaxFrameBytes);
static_assert(kErrorFrameBytes <= kMaxFrameBytes);
static_assert(kDeviceInfoMaxFrameBytes <= kMaxFrameBytes);
static_assert(kSetWifiCredentialsMaxFrameBytes == 105);
static_assert(kSetWifiCredentialsMaxFrameBytes <= kMaxFrameBytes);

[[nodiscard]] bool write_header(ByteWriter& writer, MessageType type, uint16_t request_id, uint16_t body_length)
{
    return writer.write_u8(kProtocolVersion) && writer.write_u8(static_cast<uint8_t>(type))
        && writer.write_u8(kFlagsNone) && writer.write_u16(request_id) && writer.write_u16(body_length);
}

}  // namespace

std::size_t encode_get_device_info(uint16_t request_id, std::span<uint8_t> out)
{
    constexpr std::size_t kFrameBytes = kHeaderBytes;
    if (out.size() < kFrameBytes) {
        return 0;
    }
    ByteWriter writer(out);
    if (!write_header(writer, MessageType::GetDeviceInfo, request_id, 0) || writer.written() != kFrameBytes) {
        return 0;
    }
    return kFrameBytes;
}

std::size_t encode_set_wifi_credentials(uint16_t request_id, const SetWifiCredentials& credentials,
                                        std::span<uint8_t> out)
{
    const std::size_t ssid_size = credentials.ssid.size();
    const std::size_t passphrase_size = credentials.passphrase.size();
    if (ssid_size < 1 || ssid_size > kMaxSsidBytes || passphrase_size > kMaxPassphraseBytes) {
        return 0;
    }

    const std::size_t body_length = 2 + ssid_size + passphrase_size;
    const std::size_t frame_length = kHeaderBytes + body_length;
    if (out.size() < frame_length) {
        return 0;
    }

    ByteWriter writer(out);
    if (!write_header(writer, MessageType::SetWifiCredentials, request_id, static_cast<uint16_t>(body_length))
        || !writer.write_u8(static_cast<uint8_t>(ssid_size)) || !writer.write_bytes(credentials.ssid.view())
        || !writer.write_u8(static_cast<uint8_t>(passphrase_size)) || !writer.write_bytes(credentials.passphrase.view())
        || writer.written() != frame_length) {
        return 0;
    }
    return frame_length;
}

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
    if (!write_header(writer, MessageType::DeviceInfo, request_id, static_cast<uint16_t>(body_length))
        || !writer.write_u8(info.protocol_version) || !writer.write_u8(static_cast<uint8_t>(version_size))
        || !writer.write_bytes(info.firmware_version.view()) || writer.written() != frame_length) {
        return 0;
    }
    return frame_length;
}

std::size_t encode_ack(uint16_t request_id, const Ack& ack, std::span<uint8_t> out)
{
    if (out.size() < kAckFrameBytes) {
        return 0;
    }
    ByteWriter writer(out);
    if (!write_header(writer, MessageType::Ack, request_id, 1)
        || !writer.write_u8(static_cast<uint8_t>(ack.status)) || writer.written() != kAckFrameBytes) {
        return 0;
    }
    return kAckFrameBytes;
}

std::size_t encode_error(uint16_t request_id, const Error& error, std::span<uint8_t> out)
{
    if (out.size() < kErrorFrameBytes) {
        return 0;
    }
    ByteWriter writer(out);
    if (!write_header(writer, MessageType::Error, request_id, 2)
        || !writer.write_u8(static_cast<uint8_t>(error.code))
        || !writer.write_u8(static_cast<uint8_t>(error.failed_message_type)) || writer.written() != kErrorFrameBytes) {
        return 0;
    }
    return kErrorFrameBytes;
}

Status decode_get_device_info(std::span<const uint8_t> body)
{
    ByteReader reader(body);
    if (!reader.empty()) {
        return Status::MalformedBody;
    }
    return Status::Ok;
}

Status decode_set_wifi_credentials(std::span<const uint8_t> body, SetWifiCredentials& credentials)
{
    ByteReader reader(body);
    uint8_t ssid_length = 0;
    if (!reader.read_u8(ssid_length)) {
        return Status::MalformedBody;
    }
    if (ssid_length < 1 || ssid_length > kMaxSsidBytes) {
        return Status::InvalidField;
    }

    std::array<uint8_t, kMaxSsidBytes> ssid_bytes{};
    if (!reader.read_bytes(std::span<uint8_t>(ssid_bytes.data(), ssid_length))) {
        return Status::MalformedBody;
    }

    uint8_t passphrase_length = 0;
    if (!reader.read_u8(passphrase_length)) {
        return Status::MalformedBody;
    }
    if (passphrase_length > kMaxPassphraseBytes) {
        return Status::InvalidField;
    }

    std::array<uint8_t, kMaxPassphraseBytes> passphrase_bytes{};
    if (!reader.read_bytes(std::span<uint8_t>(passphrase_bytes.data(), passphrase_length)) || !reader.empty()) {
        return Status::MalformedBody;
    }

    SetWifiCredentials decoded{};
    if (!decoded.ssid.assign(std::span<const uint8_t>(ssid_bytes.data(), ssid_length))
        || !decoded.passphrase.assign(std::span<const uint8_t>(passphrase_bytes.data(), passphrase_length))) {
        return Status::InvalidField;
    }
    credentials = decoded;
    return Status::Ok;
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
