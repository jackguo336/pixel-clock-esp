#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace ble_link {

// The text form is what iOS passes to CBUUID. The byte form is that text
// reversed, which is the order the UUID travels on the air.

inline constexpr char kDeviceName[] = "Pixel Clock";

inline constexpr std::string_view kCompanionServiceUuidText{"0833bf0e-f5b9-48aa-b1e5-f2302081b62f"};
inline constexpr std::array<uint8_t, 16> kCompanionServiceUuid{
    0x2f, 0xb6, 0x81, 0x20, 0x30, 0xf2, 0xe5, 0xb1, 0xaa, 0x48, 0xb9, 0xf5, 0x0e, 0xbf, 0x33, 0x08,
};

/// The companion writes a request here.
inline constexpr std::string_view kInboundCharacteristicUuidText{"8477f4a4-5ec0-4ee8-9721-5c0f922cd089"};
inline constexpr std::array<uint8_t, 16> kInboundCharacteristicUuid{
    0x89, 0xd0, 0x2c, 0x92, 0x0f, 0x5c, 0x21, 0x97, 0xe8, 0x4e, 0xc0, 0x5e, 0xa4, 0xf4, 0x77, 0x84,
};

/// The clock notifies a reply here.
inline constexpr std::string_view kOutboundCharacteristicUuidText{"929fa0c1-08a7-4b77-bd04-a62a88652607"};
inline constexpr std::array<uint8_t, 16> kOutboundCharacteristicUuid{
    0x07, 0x26, 0x65, 0x88, 0x2a, 0xa6, 0x04, 0xbd, 0x77, 0x4b, 0xa7, 0x08, 0xc1, 0xa0, 0x9f, 0x92,
};

}  // namespace ble_link
