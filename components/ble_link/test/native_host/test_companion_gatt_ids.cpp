#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "ble_link/companion_gatt_ids.hpp"
#include "gtest/gtest.h"

namespace {

std::optional<uint8_t> hex_digit_value(char digit)
{
    if (digit >= '0' && digit <= '9') {
        return static_cast<uint8_t>(digit - '0');
    }
    if (digit >= 'a' && digit <= 'f') {
        return static_cast<uint8_t>(digit - 'a' + 10);
    }
    return std::nullopt;
}

// Reads canonical 8-4-4-4-12 UUID text and returns its bytes in air order,
// which is the text order reversed.
std::optional<std::array<uint8_t, 16>> uuid_text_to_air_bytes(std::string_view text)
{
    constexpr std::size_t kCanonicalUuidTextLength = 36;
    if (text.size() != kCanonicalUuidTextLength) {
        return std::nullopt;
    }

    std::array<uint8_t, 16> text_order{};
    std::size_t byte_count = 0;
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '-') {
            ++i;
            continue;
        }
        const std::optional<uint8_t> high = hex_digit_value(text[i]);
        const std::optional<uint8_t> low = i + 1 < text.size() ? hex_digit_value(text[i + 1]) : std::nullopt;
        if (!high || !low || byte_count == text_order.size()) {
            return std::nullopt;
        }
        text_order[byte_count++] = static_cast<uint8_t>((*high << 4) | *low);
        i += 2;
    }
    if (byte_count != text_order.size()) {
        return std::nullopt;
    }

    std::array<uint8_t, 16> air_order{};
    for (std::size_t i = 0; i < text_order.size(); ++i) {
        air_order[i] = text_order[text_order.size() - 1 - i];
    }
    return air_order;
}

}  // namespace

TEST(CompanionGattIds, ServiceUuidTextReversesToAirBytes)
{
    EXPECT_EQ(uuid_text_to_air_bytes(ble_link::kCompanionServiceUuidText), ble_link::kCompanionServiceUuid);
}

TEST(CompanionGattIds, InboundCharacteristicUuidTextReversesToAirBytes)
{
    EXPECT_EQ(uuid_text_to_air_bytes(ble_link::kInboundCharacteristicUuidText),
              ble_link::kInboundCharacteristicUuid);
}

TEST(CompanionGattIds, OutboundCharacteristicUuidTextReversesToAirBytes)
{
    EXPECT_EQ(uuid_text_to_air_bytes(ble_link::kOutboundCharacteristicUuidText),
              ble_link::kOutboundCharacteristicUuid);
}

TEST(CompanionGattIds, DeviceNameIsPixelClock)
{
    EXPECT_EQ(std::string_view{ble_link::kDeviceName}, "Pixel Clock");
}
