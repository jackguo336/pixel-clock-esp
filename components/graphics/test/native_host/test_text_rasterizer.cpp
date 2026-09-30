#include <array>

#include "font.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"
#include "utils.hpp"
#include "text_rasterizer.hpp"

using graphics_test::expect_rgb_at;
using graphics_test::expect_size;
using graphics_test::framebuffers_equal;
using graphics_test::kBitmapPath;
using graphics_test::make_font;
using graphics_test::make_lookup_font;

namespace {

constexpr display::RgbColor kBackground{.red = 9, .green = 9, .blue = 9};
constexpr display::RgbColor kForeground{.red = 0, .green = 200, .blue = 0};
constexpr display::RgbColor kBitmapMark{.red = 255, .green = 0, .blue = 0};
constexpr display::RgbColor kBlack{};

}  // namespace

TEST(TextRasterizer, LeavesOneTransparentPixelBetweenCharacters)
{
    const std::array<display::RgbColor, 3> pixels{
        display::RgbColor{.red = 255, .green = 255, .blue = 255},
        kBitmapMark,
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const display::Font font =
        make_font({.width = 1, .height = 1}, "AB", pixels, {.width = 1, .height = 3}, kBitmapPath);
    ASSERT_TRUE(font.is_valid());

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const display::TextRasterizer rasterizer;
    const display::Size size =
        rasterizer.rasterize(font, "AB", kForeground, {.x = 2, .y = 3}, framebuffer);

    expect_size(size, 3, 1);
    expect_rgb_at(framebuffer, 2, 3, 0, 200, 0);
    expect_rgb_at(framebuffer, 3, 3, 9, 9, 9);
    expect_rgb_at(framebuffer, 4, 3, 0, 200, 0);
    expect_rgb_at(framebuffer, 2, 4, 9, 9, 9);
    expect_rgb_at(framebuffer, 4, 4, 9, 9, 9);
}

TEST(TextRasterizer, AdvancesUnknownCharactersAsBlankCharacters)
{
    const std::array<display::RgbColor, 10> pixels{
        kBitmapMark, kBlack, kBlack, kBitmapMark, kBitmapMark, kBitmapMark, kBlack, kBitmapMark, kBitmapMark, kBlack,
    };
    const display::Font font = make_lookup_font(pixels);
    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const display::TextRasterizer rasterizer;
    const display::Size size = rasterizer.rasterize(font, "A?B", kForeground, {}, framebuffer);

    expect_size(size, 8, 2);
    expect_rgb_at(framebuffer, 1, 0, 0, 200, 0);
    expect_rgb_at(framebuffer, 0, 1, 0, 200, 0);
    expect_rgb_at(framebuffer, 3, 0, 9, 9, 9);
    expect_rgb_at(framebuffer, 4, 0, 9, 9, 9);
    expect_rgb_at(framebuffer, 3, 1, 9, 9, 9);
    expect_rgb_at(framebuffer, 4, 1, 9, 9, 9);
    expect_rgb_at(framebuffer, 6, 0, 0, 200, 0);
    expect_rgb_at(framebuffer, 7, 1, 0, 200, 0);
    expect_rgb_at(framebuffer, 5, 0, 9, 9, 9);
}

TEST(TextRasterizer, ReturnsZeroSizeAndDrawsNothingForEmptyTextOrInvalidFont)
{
    const std::array<display::RgbColor, 1> pixels{kBitmapMark};
    const display::Font font = make_font({.width = 1, .height = 1}, "A", pixels, {.width = 1, .height = 1});
    const display::Font invalid_font{};

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const display::LogicalFramebuffer original = framebuffer;
    const display::TextRasterizer rasterizer;

    expect_size(rasterizer.rasterize(font, "", kForeground, {}, framebuffer), 0, 0);
    expect_size(rasterizer.rasterize(invalid_font, "A", kForeground, {}, framebuffer), 0, 0);
    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

TEST(TextRasterizer, ClipsEachCanvasEdge)
{
    const std::array<display::RgbColor, 4> pixels{kBitmapMark, kBlack, kBlack, kBitmapMark};
    const display::Font font =
        make_font({.width = 2, .height = 2}, "Q", pixels, {.width = 2, .height = 2}, kBitmapPath);
    ASSERT_TRUE(font.is_valid());
    const display::TextRasterizer rasterizer;

    {
        display::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        expect_size(rasterizer.rasterize(font, "Q", kForeground, {.x = -1, .y = 0}, framebuffer), 2, 2);
        expect_rgb_at(framebuffer, 0, 0, 9, 9, 9);
        expect_rgb_at(framebuffer, 0, 1, 0, 200, 0);
        expect_rgb_at(framebuffer, 1, 0, 9, 9, 9);
    }
    {
        display::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        expect_size(rasterizer.rasterize(font, "Q", kForeground, {.x = 0, .y = -1}, framebuffer), 2, 2);
        expect_rgb_at(framebuffer, 0, 0, 9, 9, 9);
        expect_rgb_at(framebuffer, 1, 0, 0, 200, 0);
        expect_rgb_at(framebuffer, 0, 1, 9, 9, 9);
    }
    {
        display::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        expect_size(rasterizer.rasterize(font, "Q", kForeground, {.x = 31, .y = 0}, framebuffer), 2, 2);
        expect_rgb_at(framebuffer, 31, 0, 0, 200, 0);
        expect_rgb_at(framebuffer, 31, 1, 9, 9, 9);
        expect_rgb_at(framebuffer, 30, 0, 9, 9, 9);
    }
    {
        display::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        expect_size(rasterizer.rasterize(font, "Q", kForeground, {.x = 0, .y = 7}, framebuffer), 2, 2);
        expect_rgb_at(framebuffer, 0, 7, 0, 200, 0);
        expect_rgb_at(framebuffer, 1, 7, 9, 9, 9);
        expect_rgb_at(framebuffer, 0, 6, 9, 9, 9);
    }
}
