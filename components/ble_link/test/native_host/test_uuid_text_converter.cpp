#include <array>
#include <cstdint>
#include <string_view>

#include "ble_link/companion_gatt_ids.hpp"
#include "ble_link/uuid_text_converter.hpp"
#include "gtest/gtest.h"

namespace {

constexpr std::array<uint8_t, 16> kServiceLittleEndian{
    0x2f, 0xb6, 0x81, 0x20, 0x30, 0xf2, 0xe5, 0xb1, 0xaa, 0x48, 0xb9, 0xf5, 0x0e, 0xbf, 0x33, 0x08,
};
constexpr std::array<uint8_t, 16> kInboundLittleEndian{
    0x89, 0xd0, 0x2c, 0x92, 0x0f, 0x5c, 0x21, 0x97, 0xe8, 0x4e, 0xc0, 0x5e, 0xa4, 0xf4, 0x77, 0x84,
};
constexpr std::array<uint8_t, 16> kOutboundLittleEndian{
    0x07, 0x26, 0x65, 0x88, 0x2a, 0xa6, 0x04, 0xbd, 0x77, 0x4b, 0xa7, 0x08, 0xc1, 0xa0, 0x9f, 0x92,
};

// Bluetooth base UUID, 00000000-0000-1000-8000-00805f9b34fb, little-endian.
constexpr std::string_view kBluetoothBaseUuidText{"00000000-0000-1000-8000-00805f9b34fb"};
constexpr std::array<uint8_t, 16> kBluetoothBaseLittleEndian{
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

}  // namespace

TEST(UuidTextConverter, CompanionServiceUuidIsLittleEndian)
{
    constexpr auto bytes = ble_link::uuid_text_to_little_endian_bytes(ble_link::kCompanionServiceUuidText);
    EXPECT_EQ(bytes, kServiceLittleEndian);
}

TEST(UuidTextConverter, InboundCharacteristicUuidIsLittleEndian)
{
    constexpr auto bytes = ble_link::uuid_text_to_little_endian_bytes(ble_link::kInboundCharacteristicUuidText);
    EXPECT_EQ(bytes, kInboundLittleEndian);
}

TEST(UuidTextConverter, OutboundCharacteristicUuidIsLittleEndian)
{
    constexpr auto bytes = ble_link::uuid_text_to_little_endian_bytes(ble_link::kOutboundCharacteristicUuidText);
    EXPECT_EQ(bytes, kOutboundLittleEndian);
}

TEST(UuidTextConverter, BluetoothBaseUuidIsLittleEndian)
{
    constexpr auto bytes = ble_link::uuid_text_to_little_endian_bytes(kBluetoothBaseUuidText);
    EXPECT_EQ(bytes, kBluetoothBaseLittleEndian);
}
