#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "companion_protocol/bounded_bytes.hpp"
#include "companion_protocol/set_wifi_credentials.hpp"
#include "gtest/gtest.h"
#include "utils.hpp"

using companion_protocol_test::expect_same_bytes;

namespace {

using companion_protocol::BoundedBytes;
using companion_protocol::kMaxSsidBytes;

constexpr uint8_t kRejectedByte = 0xAB;

}  // namespace

TEST(BoundedBytes, AssignRejectsOverflow)
{
    constexpr std::string_view kPrevious = "previous";
    BoundedBytes<kMaxSsidBytes> field{};
    ASSERT_TRUE(field.assign(kPrevious));

    std::array<uint8_t, kMaxSsidBytes + 1> too_long{};
    too_long.fill(kRejectedByte);
    EXPECT_FALSE(field.assign(std::span<const uint8_t>(too_long)));
    EXPECT_EQ(field.size(), kPrevious.size());
    expect_same_bytes(field.view(), std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(kPrevious.data()),
                                                             kPrevious.size()));

    const std::string too_long_text(kMaxSsidBytes + 1, '0');
    EXPECT_FALSE(field.assign(std::string_view{too_long_text}));
    EXPECT_EQ(field.size(), kPrevious.size());
    EXPECT_EQ(field.capacity(), kMaxSsidBytes);
}
