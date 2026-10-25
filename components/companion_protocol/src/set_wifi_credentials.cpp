#include "companion_protocol/set_wifi_credentials.hpp"

#include <array>

#include "byte_reader.hpp"
#include "byte_writer.hpp"

namespace companion_protocol {

std::optional<std::size_t> SetWifiCredentials::encoded_body_size() const
{
    const std::size_t ssid_size = ssid.size();
    const std::size_t passphrase_size = passphrase.size();
    if (ssid_size < kMinSsidBytes || ssid_size > kMaxSsidBytes || passphrase_size > kMaxPassphraseBytes) {
        return std::nullopt;
    }
    return kSsidLengthPrefixBytes + ssid_size + kPassphraseLengthPrefixBytes + passphrase_size;
}

bool SetWifiCredentials::encode(std::span<uint8_t> out) const
{
    const std::optional<std::size_t> body_size = encoded_body_size();
    if (!body_size.has_value() || out.size() < *body_size) {
        return false;
    }

    ByteWriter writer(out);
    return writer.write_u8(static_cast<uint8_t>(ssid.size())) && writer.write_bytes(ssid.view())
        && writer.write_u8(static_cast<uint8_t>(passphrase.size())) && writer.write_bytes(passphrase.view())
        && writer.written() == *body_size;
}

Status SetWifiCredentials::decode(std::span<const uint8_t> bytes)
{
    ByteReader reader(bytes);
    uint8_t ssid_length = 0;
    if (!reader.read_u8(ssid_length)) {
        return Status::MalformedBody;
    }
    if (ssid_length < kMinSsidBytes || ssid_length > kMaxSsidBytes) {
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
    *this = decoded;
    return Status::Ok;
}

}  // namespace companion_protocol
