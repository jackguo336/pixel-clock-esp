#include <array>
#include <cstdint>

#include "display/bitmap.hpp"
#include "gtest/gtest.h"

namespace {

constexpr display::RgbColor kRed{255, 0, 0};
constexpr display::RgbColor kGreen{0, 255, 0};

}  // namespace

TEST(BitmapView, AcceptsNonZeroDimensionsWithExactPixelCount)
{
    const std::array<display::RgbColor, 6> pixels{kRed, kGreen, kRed, kGreen, kRed, kGreen};
    const display::BitmapView view{
        .size = {.width = 3, .height = 2},
        .pixels = pixels,
    };

    EXPECT_TRUE(view.is_valid());
}

TEST(BitmapView, RejectsZeroWidth)
{
    const std::array<display::RgbColor, 2> pixels{kRed, kGreen};
    const display::BitmapView view{
        .size = {.width = 0, .height = 2},
        .pixels = pixels,
    };

    EXPECT_FALSE(view.is_valid());
}

TEST(BitmapView, RejectsZeroHeight)
{
    const std::array<display::RgbColor, 2> pixels{kRed, kGreen};
    const display::BitmapView view{
        .size = {.width = 2, .height = 0},
        .pixels = pixels,
    };

    EXPECT_FALSE(view.is_valid());
}

TEST(BitmapView, RejectsUndersizedSpan)
{
    const std::array<display::RgbColor, 3> pixels{kRed, kGreen, kRed};
    const display::BitmapView view{
        .size = {.width = 2, .height = 2},
        .pixels = pixels,
    };

    EXPECT_FALSE(view.is_valid());
}

TEST(BitmapView, RejectsOversizedSpan)
{
    const std::array<display::RgbColor, 5> pixels{kRed, kGreen, kRed, kGreen, kRed};
    const display::BitmapView view{
        .size = {.width = 2, .height = 2},
        .pixels = pixels,
    };

    EXPECT_FALSE(view.is_valid());
}

TEST(MutableBitmapView, ConvertsToReadOnlyWithoutChangingStorage)
{
    std::array<display::RgbColor, 4> pixels{kRed, kGreen, kRed, kGreen};
    const display::MutableBitmapView mutable_view{
        .size = {.width = 2, .height = 2},
        .pixels = pixels,
    };

    const display::BitmapView view = mutable_view.as_read_only();

    EXPECT_TRUE(mutable_view.is_valid());
    EXPECT_TRUE(view.is_valid());
    EXPECT_EQ(view.size.width, mutable_view.size.width);
    EXPECT_EQ(view.size.height, mutable_view.size.height);
    EXPECT_EQ(view.pixels.data(), pixels.data());
    EXPECT_EQ(view.pixels.size(), pixels.size());
    EXPECT_EQ(view.pixels.data(), mutable_view.pixels.data());
}
