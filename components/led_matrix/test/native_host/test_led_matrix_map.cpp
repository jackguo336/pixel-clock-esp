#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>

#include "display/logical_framebuffer.hpp"
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

TEST(LedMatrixMap, HorizontalCornersAndInterior)
{
    constexpr auto orientation = led_matrix::LedMatrixOrientation::Horizontal;
    expect_index(0, 0, orientation, false, 0);
    expect_index(31, 0, orientation, false, 31);
    expect_index(0, 7, orientation, false, 224);
    expect_index(31, 7, orientation, false, 255);
    expect_index(15, 3, orientation, false, 111);
}

TEST(LedMatrixMap, HorizontalReversedCornersAndInterior)
{
    constexpr auto orientation = led_matrix::LedMatrixOrientation::HorizontalReversed;
    expect_index(31, 0, orientation, false, 0);
    expect_index(0, 0, orientation, false, 31);
    expect_index(31, 7, orientation, false, 224);
    expect_index(0, 7, orientation, false, 255);
    expect_index(15, 3, orientation, false, 112);
}

TEST(LedMatrixMap, VerticalCornersAndInterior)
{
    constexpr auto orientation = led_matrix::LedMatrixOrientation::Vertical;
    expect_index(0, 0, orientation, false, 0);
    expect_index(0, 7, orientation, false, 7);
    expect_index(31, 0, orientation, false, 248);
    expect_index(31, 7, orientation, false, 255);
    expect_index(15, 3, orientation, false, 123);
}

TEST(LedMatrixMap, VerticalReversedCornersAndInterior)
{
    constexpr auto orientation = led_matrix::LedMatrixOrientation::VerticalReversed;
    expect_index(0, 7, orientation, false, 0);
    expect_index(0, 0, orientation, false, 7);
    expect_index(31, 7, orientation, false, 248);
    expect_index(31, 0, orientation, false, 255);
    expect_index(15, 3, orientation, false, 124);
}

TEST(LedMatrixMap, SerpentineReversesOddLanes)
{
    expect_index(0, 1, led_matrix::LedMatrixOrientation::Horizontal, true, 63);
    expect_index(31, 1, led_matrix::LedMatrixOrientation::Horizontal, true, 32);
    expect_index(0, 7, led_matrix::LedMatrixOrientation::Horizontal, true, 255);
    expect_index(31, 7, led_matrix::LedMatrixOrientation::Horizontal, true, 224);
    expect_index(15, 3, led_matrix::LedMatrixOrientation::Horizontal, true, 112);

    expect_index(0, 1, led_matrix::LedMatrixOrientation::HorizontalReversed, true, 32);
    expect_index(31, 1, led_matrix::LedMatrixOrientation::HorizontalReversed, true, 63);
    expect_index(15, 3, led_matrix::LedMatrixOrientation::HorizontalReversed, true, 111);

    expect_index(1, 0, led_matrix::LedMatrixOrientation::Vertical, true, 15);
    expect_index(1, 7, led_matrix::LedMatrixOrientation::Vertical, true, 8);
    expect_index(31, 0, led_matrix::LedMatrixOrientation::Vertical, true, 255);
    expect_index(31, 7, led_matrix::LedMatrixOrientation::Vertical, true, 248);
    expect_index(15, 3, led_matrix::LedMatrixOrientation::Vertical, true, 124);

    expect_index(1, 0, led_matrix::LedMatrixOrientation::VerticalReversed, true, 8);
    expect_index(1, 7, led_matrix::LedMatrixOrientation::VerticalReversed, true, 15);
    expect_index(31, 0, led_matrix::LedMatrixOrientation::VerticalReversed, true, 248);
    expect_index(31, 7, led_matrix::LedMatrixOrientation::VerticalReversed, true, 255);
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
