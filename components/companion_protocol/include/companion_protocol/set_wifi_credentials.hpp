#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "companion_protocol/bounded_bytes.hpp"
#include "companion_protocol/message_header.hpp"

namespace companion_protocol {

inline constexpr std::size_t kMinSsidBytes = 1;
inline constexpr std::size_t kMaxSsidBytes = 32;
inline constexpr std::size_t kMaxPassphraseBytes = 64;
inline constexpr std::size_t kSsidLengthPrefixBytes = 1;
inline constexpr std::size_t kPassphraseLengthPrefixBytes = 1;

// Keep PROTOCOL.md in sync with this message.
struct SetWifiCredentials {
    static constexpr MessageType kMessageType = MessageType::SetWifiCredentials;

    BoundedBytes<kMaxSsidBytes> ssid;
    BoundedBytes<kMaxPassphraseBytes> passphrase;

    // nullopt when the SSID is empty or longer than kMaxSsidBytes, or the
    // passphrase is longer than kMaxPassphraseBytes.
    [[nodiscard]] std::optional<std::size_t> encoded_body_size() const;

    [[nodiscard]] bool encode(std::span<uint8_t> out) const;

    [[nodiscard]] Status decode(std::span<const uint8_t> bytes);
};

}  // namespace companion_protocol
