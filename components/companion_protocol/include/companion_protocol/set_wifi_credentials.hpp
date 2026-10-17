#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "companion_protocol/bounded_bytes.hpp"
#include "companion_protocol/frame.hpp"

namespace companion_protocol {

inline constexpr std::size_t kMinSsidBytes = 1;
inline constexpr std::size_t kMaxSsidBytes = 32;
inline constexpr std::size_t kMaxPassphraseBytes = 64;
inline constexpr std::size_t kSsidLengthPrefixBytes = 1;
inline constexpr std::size_t kPassphraseLengthPrefixBytes = 1;
inline constexpr std::size_t kSetWifiCredentialsMaxFrameBytes =
    kHeaderBytes + kSsidLengthPrefixBytes + kMaxSsidBytes + kPassphraseLengthPrefixBytes + kMaxPassphraseBytes;
// PROTOCOL.md documents this SetWifiCredentials frame as the largest v1 frame.
inline constexpr std::size_t kLargestV1FrameBytes = 105;

static_assert(kSetWifiCredentialsMaxFrameBytes == kLargestV1FrameBytes);
static_assert(kSetWifiCredentialsMaxFrameBytes <= kMaxFrameBytes);

struct SetWifiCredentials {
    BoundedBytes<kMaxSsidBytes> ssid;
    BoundedBytes<kMaxPassphraseBytes> passphrase;
};

// Writes a SetWifiCredentials frame. Returns the frame length, or 0 if `out` is
// too small, the SSID is empty or longer than kMaxSsidBytes, or the passphrase
// is longer than kMaxPassphraseBytes.
[[nodiscard]] std::size_t encode_set_wifi_credentials(uint16_t request_id, const SetWifiCredentials& credentials,
                                                      std::span<uint8_t> out);

// Copies SSID and passphrase into `credentials`. The previous value is left unchanged on failure.
[[nodiscard]] Status decode_set_wifi_credentials(std::span<const uint8_t> body, SetWifiCredentials& credentials);

}  // namespace companion_protocol
