#include "companion_protocol/set_wifi_credentials.hpp"

#include <array>

#include "byte_cursor.hpp"
#include "write_frame_header.hpp"

namespace companion_protocol {
namespace {

// Header + ssid_length + 32 SSID bytes + passphrase_length + 64 passphrase bytes.
constexpr std::size_t kSetWifiCredentialsMaxFrameBytes = kHeaderBytes + 1 + kMaxSsidBytes + 1 + kMaxPassphraseBytes;

static_assert(kSetWifiCredentialsMaxFrameBytes == 105);
static_assert(kSetWifiCredentialsMaxFrameBytes <= kMaxFrameBytes);

}  // namespace

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
    if (!write_frame_header(writer, MessageType::SetWifiCredentials, request_id, static_cast<uint16_t>(body_length))
        || !writer.write_u8(static_cast<uint8_t>(ssid_size)) || !writer.write_bytes(credentials.ssid.view())
        || !writer.write_u8(static_cast<uint8_t>(passphrase_size)) || !writer.write_bytes(credentials.passphrase.view())
        || writer.written() != frame_length) {
        return 0;
    }
    return frame_length;
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

}  // namespace companion_protocol
