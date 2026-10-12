#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "companion_protocol/frame.hpp"

namespace companion_protocol {

// Writes a GetDeviceInfo frame. Returns the frame length, or 0 if `out` is too small.
[[nodiscard]] std::size_t encode_get_device_info(uint16_t request_id, std::span<uint8_t> out);

// Ok when `body` is empty. Any other size is MalformedBody.
[[nodiscard]] Status decode_get_device_info(std::span<const uint8_t> body);

}  // namespace companion_protocol
