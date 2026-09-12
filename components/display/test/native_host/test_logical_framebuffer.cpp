#include <cstddef>
#include <cstdint>

#include "display/elements.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"

namespace {

void expect_rgb(const display::RgbColor& actual, uint8_t red, uint8_t green, uint8_t blue)
{
    EXPECT_EQ(actual.red, red);
    EXPECT_EQ(actual.green, green);
    EXPECT_EQ(actual.blue, blue);
}

void expect_all_pixels(const display::LogicalFramebuffer& framebuffer, uint8_t red, uint8_t green, uint8_t blue)
{
    const auto pixels = framebuffer.pixels();
    ASSERT_EQ(pixels.size(), display::LogicalFramebuffer::kPixelCount);
    for (const display::RgbColor& pixel : pixels) {
        expect_rgb(pixel, red, green, blue);
    }
}

}  // namespace

TEST(LogicalFramebuffer, StartsWithEveryPixelBlack)
{
    const display::LogicalFramebuffer framebuffer;
    expect_all_pixels(framebuffer, 0, 0, 0);
}

TEST(LogicalFramebuffer, ClearFillsEveryLogicalPixel)
{
    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 12, .green = 34, .blue = 56});
    expect_all_pixels(framebuffer, 12, 34, 56);
}

TEST(LogicalFramebuffer, WritesAndReadsCornerAndInteriorCoordinates)
{
    display::LogicalFramebuffer framebuffer;
    EXPECT_TRUE(framebuffer.set_pixel(0, 0, display::RgbColor{.red = 1, .green = 0, .blue = 0}));
    EXPECT_TRUE(framebuffer.set_pixel(31, 0, display::RgbColor{.red = 0, .green = 1, .blue = 0}));
    EXPECT_TRUE(framebuffer.set_pixel(0, 7, display::RgbColor{.red = 0, .green = 0, .blue = 1}));
    EXPECT_TRUE(framebuffer.set_pixel(31, 7, display::RgbColor{.red = 1, .green = 1, .blue = 0}));
    EXPECT_TRUE(framebuffer.set_pixel(15, 3, display::RgbColor{.red = 1, .green = 0, .blue = 1}));

    const display::RgbColor* top_left = framebuffer.pixel_at(0, 0);
    const display::RgbColor* top_right = framebuffer.pixel_at(31, 0);
    const display::RgbColor* bottom_left = framebuffer.pixel_at(0, 7);
    const display::RgbColor* bottom_right = framebuffer.pixel_at(31, 7);
    const display::RgbColor* interior = framebuffer.pixel_at(15, 3);
    ASSERT_NE(top_left, nullptr);
    ASSERT_NE(top_right, nullptr);
    ASSERT_NE(bottom_left, nullptr);
    ASSERT_NE(bottom_right, nullptr);
    ASSERT_NE(interior, nullptr);
    expect_rgb(*top_left, 1, 0, 0);
    expect_rgb(*top_right, 0, 1, 0);
    expect_rgb(*bottom_left, 0, 0, 1);
    expect_rgb(*bottom_right, 1, 1, 0);
    expect_rgb(*interior, 1, 0, 1);
}

TEST(LogicalFramebuffer, RejectsOutOfRangeCoordinatesWithoutChangingValidPixels)
{
    display::LogicalFramebuffer framebuffer;
    ASSERT_TRUE(framebuffer.set_pixel(4, 2, display::RgbColor{.red = 9, .green = 8, .blue = 7}));

    EXPECT_FALSE(framebuffer.set_pixel(-1, 0, display::RgbColor{.red = 255, .green = 0, .blue = 0}));
    EXPECT_FALSE(framebuffer.set_pixel(0, -1, display::RgbColor{.red = 255, .green = 0, .blue = 0}));
    EXPECT_FALSE(framebuffer.set_pixel(32, 0, display::RgbColor{.red = 255, .green = 0, .blue = 0}));
    EXPECT_FALSE(framebuffer.set_pixel(0, 8, display::RgbColor{.red = 255, .green = 0, .blue = 0}));
    EXPECT_EQ(framebuffer.pixel_at(-1, 0), nullptr);
    EXPECT_EQ(framebuffer.pixel_at(0, -1), nullptr);
    EXPECT_EQ(framebuffer.pixel_at(32, 0), nullptr);
    EXPECT_EQ(framebuffer.pixel_at(0, 8), nullptr);

    const display::RgbColor* kept = framebuffer.pixel_at(4, 2);
    ASSERT_NE(kept, nullptr);
    expect_rgb(*kept, 9, 8, 7);
}

TEST(LogicalFramebuffer, ExposesPixelsInRowMajorLogicalOrder)
{
    display::LogicalFramebuffer framebuffer;
    ASSERT_TRUE(framebuffer.set_pixel(0, 0, display::RgbColor{.red = 1, .green = 0, .blue = 0}));
    ASSERT_TRUE(framebuffer.set_pixel(1, 0, display::RgbColor{.red = 2, .green = 0, .blue = 0}));
    ASSERT_TRUE(framebuffer.set_pixel(0, 1, display::RgbColor{.red = 3, .green = 0, .blue = 0}));

    const auto pixels = framebuffer.pixels();
    ASSERT_EQ(pixels.size(), display::LogicalFramebuffer::kPixelCount);
    expect_rgb(pixels[0], 1, 0, 0);
    expect_rgb(pixels[1], 2, 0, 0);
    expect_rgb(pixels[static_cast<std::size_t>(display::LogicalFramebuffer::kWidth)], 3, 0, 0);
    expect_rgb(pixels[2], 0, 0, 0);
}
