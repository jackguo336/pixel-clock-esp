#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "companion_protocol/bounded_bytes.hpp"
#include "companion_protocol/frame.hpp"

namespace companion_protocol {

inline constexpr std::size_t kMaxFirmwareVersionBytes = 32;
inline constexpr std::size_t kProtocolVersionFieldBytes = 1;
inline constexpr std::size_t kFirmwareVersionLengthPrefixBytes = 1;
inline constexpr std::size_t kDeviceInfoMaxFrameBytes =
    kHeaderBytes + kProtocolVersionFieldBytes + kFirmwareVersionLengthPrefixBytes + kMaxFirmwareVersionBytes;

static_assert(kDeviceInfoMaxFrameBytes <= kMaxFrameBytes);

struct DeviceInfo {
    uint8_t protocol_version{kProtocolVersion};
    BoundedBytes<kMaxFirmwareVersionBytes> firmware_version;
};

// Writes a DeviceInfo frame. Returns the frame length, or 0 if `out` is too small.
[[nodiscard]] std::size_t encode_device_info(uint16_t request_id, const DeviceInfo& info, std::span<uint8_t> out);

// Copies protocol version and firmware version. `info` is left unchanged on failure.
[[nodiscard]] Status decode_device_info(std::span<const uint8_t> body, DeviceInfo& info);

}  // namespace companion_protocol
