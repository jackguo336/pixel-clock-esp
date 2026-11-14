#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ble_link {

namespace uuid_air_bytes_detail {

// Not constexpr and never defined: reaching it while computing a UUID constant
// stops the build.
void reject_malformed_uuid_text();

consteval uint8_t hex_digit_value(char digit)
{
    if (digit >= '0' && digit <= '9') {
        return static_cast<uint8_t>(digit - '0');
    }
    if (digit >= 'a' && digit <= 'f') {
        return static_cast<uint8_t>(digit - 'a' + 10);
    }
    reject_malformed_uuid_text();
    return 0;
}

consteval bool is_hyphen_offset(std::size_t offset)
{
    constexpr std::array<std::size_t, 4> kHyphenOffsets{8, 13, 18, 23};
    for (const std::size_t hyphen_offset : kHyphenOffsets) {
        if (offset == hyphen_offset) {
            return true;
        }
    }
    return false;
}

}  // namespace uuid_air_bytes_detail

/// Reads lowercase 8-4-4-4-12 UUID text, the form iOS passes to CBUUID, and
/// returns the bytes in the order they travel on the air: the text order
/// reversed. Malformed text fails the build.
consteval std::array<uint8_t, 16> uuid_air_bytes_from_text(std::string_view text)
{
    namespace detail = uuid_air_bytes_detail;

    constexpr std::size_t kUuidTextLength = 36;
    if (text.size() != kUuidTextLength) {
        detail::reject_malformed_uuid_text();
    }

    std::array<uint8_t, 16> air_bytes{};
    std::size_t next_air_index = air_bytes.size();
    for (std::size_t offset = 0; offset < text.size();) {
        if (detail::is_hyphen_offset(offset)) {
            if (text[offset] != '-') {
                detail::reject_malformed_uuid_text();
            }
            ++offset;
            continue;
        }
        const uint8_t high = detail::hex_digit_value(text[offset]);
        const uint8_t low = detail::hex_digit_value(text[offset + 1]);
        air_bytes[--next_air_index] = static_cast<uint8_t>((high << 4) | low);
        offset += 2;
    }
    return air_bytes;
}

}  // namespace ble_link
