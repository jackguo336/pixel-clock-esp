#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "companion_protocol/bounded_bytes.hpp"
#include "companion_protocol/frame.hpp"

namespace companion_protocol {

struct SetWifiCredentials {
    BoundedBytes<kMaxSsidBytes> ssid;
    BoundedBytes<kMaxPassphraseBytes> passphrase;
};

struct DeviceInfo {
    uint8_t protocol_version{kProtocolVersion};
    BoundedBytes<kMaxFirmwareVersionBytes> firmware_version;
};

struct Ack {
    Status status{Status::Ok};
};

struct Error {
    Status code{};
    MessageType failed_message_type{};
};

// Encoders write a full frame (header + body) into `out`, using kProtocolVersion
// and flags = 0. Each returns the frame length, or 0 if the frame does not fit
// in `out`. A valid frame is never 0 bytes. encode_set_wifi_credentials also
// returns 0 when the SSID is empty or longer than kMaxSsidBytes, or the
// passphrase is longer than kMaxPassphraseBytes.

[[nodiscard]] std::size_t encode_get_device_info(uint16_t request_id, std::span<uint8_t> out);

[[nodiscard]] std::size_t encode_set_wifi_credentials(uint16_t request_id, const SetWifiCredentials& credentials,
                                                      std::span<uint8_t> out);

[[nodiscard]] std::size_t encode_device_info(uint16_t request_id, const DeviceInfo& info, std::span<uint8_t> out);

[[nodiscard]] std::size_t encode_ack(uint16_t request_id, const Ack& ack, std::span<uint8_t> out);

[[nodiscard]] std::size_t encode_error(uint16_t request_id, const Error& error, std::span<uint8_t> out);

// Body decoders copy into bounded storage so the result outlives the frame buffer.
[[nodiscard]] Status decode_get_device_info(std::span<const uint8_t> body);

[[nodiscard]] Status decode_set_wifi_credentials(std::span<const uint8_t> body, SetWifiCredentials& credentials);

[[nodiscard]] Status decode_device_info(std::span<const uint8_t> body, DeviceInfo& info);

[[nodiscard]] Status decode_ack(std::span<const uint8_t> body, Ack& ack);

[[nodiscard]] Status decode_error(std::span<const uint8_t> body, Error& error);

}  // namespace companion_protocol
