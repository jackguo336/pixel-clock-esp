#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>

#include "logical_framebuffer.hpp"
#include "gtest/gtest.h"
#include "led_matrix_map.hpp"

namespace {

void expect_index(int32_t x, int32_t y, led_matrix::LedMatrixOrientation orientation, bool serpentine,
                  std::size_t expected)
{
    EXPECT_EQ(led_matrix::led_index_for_logical_pixel(x, y, orientation, serpentine), expected);
}

std::string orientation_name(led_matrix::LedMatrixOrientation orientation)
{
    switch (orientation) {
    case led_matrix::LedMatrixOrientation::Horizontal:
        return "Horizontal";
    case led_matrix::LedMatrixOrientation::HorizontalReversed:
        return "HorizontalReversed";
    case led_matrix::LedMatrixOrientation::Vertical:
        return "Vertical";
    case led_matrix::LedMatrixOrientation::VerticalReversed:
        return "VerticalReversed";
    }
    return "Unknown";
}

using Wiring = std::tuple<led_matrix::LedMatrixOrientation, bool>;

std::string wiring_test_name(const testing::TestParamInfo<Wiring>& info)
{
    const auto [orientation, serpentine] = info.param;
    return orientation_name(orientation) + (serpentine ? "_Serpentine" : "_Straight");
}

class LedMatrixMapWiring : public testing::TestWithParam<Wiring> {};

}  // namespace

TEST(LedMatrixMap, HorizontalRunsLeftToRight)
{
    constexpr auto orientation = led_matrix::LedMatrixOrientation::Horizontal;
    // Row 0 runs left-to-right: top-left is 0, top-right is width - 1.
    expect_index(0, 0, orientation, false, 0);
    expect_index(31, 0, orientation, false, 31);
    // A later row continues at row * width + column (3 * 32 + 15).
    expect_index(15, 3, orientation, false, 111);
}

TEST(LedMatrixMap, HorizontalReversedRunsRightToLeft)
{
    constexpr auto orientation = led_matrix::LedMatrixOrientation::HorizontalReversed;
    // Row 0 starts at the right edge, so top-right is 0 and top-left is width - 1.
    expect_index(31, 0, orientation, false, 0);
    expect_index(0, 0, orientation, false, 31);
    // A later row is counted from the right (3 * 32 + (31 - 15)).
    expect_index(15, 3, orientation, false, 112);
}

TEST(LedMatrixMap, VerticalRunsTopToBottom)
{
    constexpr auto orientation = led_matrix::LedMatrixOrientation::Vertical;
    // Column 0 runs top-to-bottom: the top is 0, the bottom is height - 1.
    expect_index(0, 0, orientation, false, 0);
    expect_index(0, 7, orientation, false, 7);
    // A later column continues at column * height + row (15 * 8 + 3).
    expect_index(15, 3, orientation, false, 123);
}

TEST(LedMatrixMap, VerticalReversedRunsBottomToTop)
{
    constexpr auto orientation = led_matrix::LedMatrixOrientation::VerticalReversed;
    // Column 0 starts at the bottom, so the bottom is 0 and the top is height - 1.
    expect_index(0, 7, orientation, false, 0);
    expect_index(0, 0, orientation, false, 7);
    // A later column is counted from the bottom (15 * 8 + (7 - 3)).
    expect_index(15, 3, orientation, false, 124);
}

TEST(LedMatrixMap, SerpentineReversesOddLanes)
{
    // First odd row runs right-to-left: left edge is 32 + 31, right edge is 32.
    expect_index(0, 1, led_matrix::LedMatrixOrientation::Horizontal, true, 63);
    expect_index(31, 1, led_matrix::LedMatrixOrientation::Horizontal, true, 32);
    // Later odd row is counted from the right (3 * 32 + (31 - 15)).
    expect_index(15, 3, led_matrix::LedMatrixOrientation::Horizontal, true, 112);

    // Reversed wiring flips that odd row back to left-to-right.
    expect_index(0, 1, led_matrix::LedMatrixOrientation::HorizontalReversed, true, 32);
    expect_index(31, 1, led_matrix::LedMatrixOrientation::HorizontalReversed, true, 63);
    expect_index(15, 3, led_matrix::LedMatrixOrientation::HorizontalReversed, true, 111);

    // First odd column runs bottom-to-top: top is 8 + 7, bottom is 8.
    expect_index(1, 0, led_matrix::LedMatrixOrientation::Vertical, true, 15);
    expect_index(1, 7, led_matrix::LedMatrixOrientation::Vertical, true, 8);
    // Later odd column is counted from the bottom (15 * 8 + (7 - 3)).
    expect_index(15, 3, led_matrix::LedMatrixOrientation::Vertical, true, 124);

    // Reversed wiring flips that odd column back to top-to-bottom.
    expect_index(1, 0, led_matrix::LedMatrixOrientation::VerticalReversed, true, 8);
    expect_index(1, 7, led_matrix::LedMatrixOrientation::VerticalReversed, true, 15);
    expect_index(15, 3, led_matrix::LedMatrixOrientation::VerticalReversed, true, 123);
}

TEST_P(LedMatrixMapWiring, UsesEachLedExactlyOnce)
{
    const auto [orientation, serpentine] = GetParam();
    std::array<int, display::LogicalFramebuffer::kPixelCount> seen{};
    for (int32_t y = 0; y < display::LogicalFramebuffer::kHeight; ++y) {
        for (int32_t x = 0; x < display::LogicalFramebuffer::kWidth; ++x) {
            const std::size_t index =
                led_matrix::led_index_for_logical_pixel(x, y, orientation, serpentine);
            ASSERT_LT(index, display::LogicalFramebuffer::kPixelCount);
            seen[index] += 1;
        }
    }
    for (const int count : seen) {
        EXPECT_EQ(count, 1);
    }
}

INSTANTIATE_TEST_SUITE_P(LedMatrixMap, LedMatrixMapWiring,
                         testing::Combine(testing::Values(led_matrix::LedMatrixOrientation::Horizontal,
                                                          led_matrix::LedMatrixOrientation::HorizontalReversed,
                                                          led_matrix::LedMatrixOrientation::Vertical,
                                                          led_matrix::LedMatrixOrientation::VerticalReversed),
                                          testing::Bool()),
                         wiring_test_name);
