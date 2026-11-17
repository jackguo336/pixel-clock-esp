#pragma once

#include <cstdint>
#include <span>

namespace ble_link {

/// Bytes of the app description version. Stops before a trailing 0, and at 32 bytes.
[[nodiscard]] std::span<const uint8_t> firmware_version_bytes();

}  // namespace ble_link
