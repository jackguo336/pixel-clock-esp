#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "companion_protocol/bounded_bytes.hpp"
#include "companion_protocol/message_header.hpp"

namespace companion_protocol {

inline constexpr std::size_t kMaxFirmwareVersionBytes = 32;
inline constexpr std::size_t kProtocolVersionFieldBytes = 1;
// The size of the version field stored in a single byte before the version field itself.
inline constexpr std::size_t kFirmwareVersionLengthPrefixBytes = 1;

// Keep PROTOCOL.md in sync with this message.
struct DeviceInfo {
    static constexpr MessageType kMessageType = MessageType::DeviceInfo;

    uint8_t protocol_version{kProtocolVersion};
    BoundedBytes<kMaxFirmwareVersionBytes> firmware_version;

    // nullopt when the firmware version is longer than kMaxFirmwareVersionBytes.
    [[nodiscard]] std::optional<std::size_t> encoded_body_size() const;

    [[nodiscard]] bool encode(std::span<uint8_t> out) const;

    [[nodiscard]] Status decode(std::span<const uint8_t> bytes);
};

}  // namespace companion_protocol
