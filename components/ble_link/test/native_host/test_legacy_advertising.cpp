#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "ble_link/companion_gatt_ids.hpp"
#include "ble_link/legacy_advertising.hpp"
#include "gtest/gtest.h"

namespace {

using ble_link::LegacyAdvertisingPackets;

constexpr uint8_t kPreviousPacketByte = 0xAB;
constexpr std::size_t kPreviousPacketLength = 7;

LegacyAdvertisingPackets previous_packets()
{
    LegacyAdvertisingPackets packets{};
    packets.advertising.fill(kPreviousPacketByte);
    packets.advertising_length = kPreviousPacketLength;
    packets.scan_response.fill(kPreviousPacketByte);
    packets.scan_response_length = kPreviousPacketLength;
    return packets;
}

void expect_packets_equal(const LegacyAdvertisingPackets& expected, const LegacyAdvertisingPackets& actual)
{
    EXPECT_EQ(expected.advertising, actual.advertising);
    EXPECT_EQ(expected.advertising_length, actual.advertising_length);
    EXPECT_EQ(expected.scan_response, actual.scan_response);
    EXPECT_EQ(expected.scan_response_length, actual.scan_response_length);
}

}  // namespace

TEST(LegacyAdvertising, AdvertisingPacketIsFlagsPlusServiceUuid)
{
    LegacyAdvertisingPackets packets{};
    ASSERT_TRUE(ble_link::build_legacy_advertising_packets(ble_link::kCompanionServiceUuid, ble_link::kDeviceName,
                                                           packets));

    const std::array<uint8_t, 21> expected = {
        0x02, 0x01, 0x06,
        0x11, 0x07,
        0x2f, 0xb6, 0x81, 0x20, 0x30, 0xf2, 0xe5, 0xb1, 0xaa, 0x48, 0xb9, 0xf5, 0x0e, 0xbf, 0x33, 0x08,
    };
    ASSERT_EQ(packets.advertising_length, expected.size());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), packets.advertising.begin()));
}

TEST(LegacyAdvertising, ScanResponseIsCompleteLocalName)
{
    LegacyAdvertisingPackets packets{};
    ASSERT_TRUE(ble_link::build_legacy_advertising_packets(ble_link::kCompanionServiceUuid, ble_link::kDeviceName,
                                                           packets));

    const std::array<uint8_t, 13> expected = {
        0x0c, 0x09, 'P', 'i', 'x', 'e', 'l', ' ', 'C', 'l', 'o', 'c', 'k',
    };
    ASSERT_EQ(packets.scan_response_length, expected.size());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), packets.scan_response.begin()));
}

TEST(LegacyAdvertising, NameThatFillsTheScanResponseFits)
{
    const std::string longest_name(ble_link::kLegacyAdvertisingPacketBytes - 2, 'n');
    LegacyAdvertisingPackets packets{};

    ASSERT_TRUE(ble_link::build_legacy_advertising_packets(ble_link::kCompanionServiceUuid, longest_name, packets));
    EXPECT_EQ(packets.scan_response_length, ble_link::kLegacyAdvertisingPacketBytes);
}

TEST(LegacyAdvertising, NameThatDoesNotFitLeavesPacketsUnchanged)
{
    const std::string too_long_name(ble_link::kLegacyAdvertisingPacketBytes - 1, 'n');
    LegacyAdvertisingPackets packets = previous_packets();

    EXPECT_FALSE(ble_link::build_legacy_advertising_packets(ble_link::kCompanionServiceUuid, too_long_name, packets));
    expect_packets_equal(previous_packets(), packets);
}

TEST(LegacyAdvertising, EmptyNameLeavesPacketsUnchanged)
{
    LegacyAdvertisingPackets packets = previous_packets();

    EXPECT_FALSE(ble_link::build_legacy_advertising_packets(ble_link::kCompanionServiceUuid, std::string_view{},
                                                            packets));
    expect_packets_equal(previous_packets(), packets);
}
