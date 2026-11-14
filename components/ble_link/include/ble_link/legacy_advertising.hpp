#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ble_link {

inline constexpr std::size_t kLegacyAdvertisingPacketBytes = 31;

struct LegacyAdvertisingPackets {
    std::array<uint8_t, kLegacyAdvertisingPacketBytes> advertising{};
    std::size_t advertising_length{0};
    std::array<uint8_t, kLegacyAdvertisingPacketBytes> scan_response{};
    std::size_t scan_response_length{0};
};

/// The advertising packet carries the flags and the service UUID. The scan
/// response carries the name.
/// Returns false when the name is empty or either packet would pass 31 bytes.
/// On false, out is left unchanged.
[[nodiscard]] bool build_legacy_advertising_packets(std::span<const uint8_t, 16> service_uuid,
                                                    std::string_view device_name,
                                                    LegacyAdvertisingPackets& out);

}  // namespace ble_link
