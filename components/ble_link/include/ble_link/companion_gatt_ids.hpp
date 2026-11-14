#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "ble_link/uuid_air_bytes.hpp"

namespace ble_link {

inline constexpr char kDeviceName[] = "Pixel Clock";

inline constexpr std::string_view kCompanionServiceUuidText{"0833bf0e-f5b9-48aa-b1e5-f2302081b62f"};
inline constexpr std::array<uint8_t, 16> kCompanionServiceUuid = uuid_air_bytes_from_text(kCompanionServiceUuidText);

/// The companion writes a request here.
inline constexpr std::string_view kInboundCharacteristicUuidText{"8477f4a4-5ec0-4ee8-9721-5c0f922cd089"};
inline constexpr std::array<uint8_t, 16> kInboundCharacteristicUuid =
    uuid_air_bytes_from_text(kInboundCharacteristicUuidText);

/// The clock notifies a reply here.
inline constexpr std::string_view kOutboundCharacteristicUuidText{"929fa0c1-08a7-4b77-bd04-a62a88652607"};
inline constexpr std::array<uint8_t, 16> kOutboundCharacteristicUuid =
    uuid_air_bytes_from_text(kOutboundCharacteristicUuidText);

}  // namespace ble_link
