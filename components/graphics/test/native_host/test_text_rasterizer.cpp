#include <array>

#include "font.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"
#include "text_measurement.hpp"
#include "text_rasterizer.hpp"
#include "utils.hpp"

using graphics_test::expect_rgb_at;
using graphics_test::expect_size;
using graphics_test::framebuffers_equal;
using graphics_test::kBitmapPath;
using graphics_test::make_font;
using graphics_test::make_lookup_font;
using graphics_test::solid_sampler;

namespace {

constexpr graphics::RgbColor kBackground{.red = 9, .green = 9, .blue = 9};
constexpr graphics::RgbColor kForeground{.red = 0, .green = 200, .blue = 0};
constexpr graphics::RgbColor kBitmapMark{.red = 255, .green = 0, .blue = 0};
constexpr graphics::RgbColor kBlack{};

}  // namespace

TEST(TextRasterizer, LeavesOneTransparentPixelBetweenCharacters)
{
    const std::array<graphics::RgbColor, 3> pixels{
        graphics::RgbColor{.red = 255, .green = 255, .blue = 255},
        kBitmapMark,
        graphics::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const graphics::Font font =
        make_font({.width = 1, .height = 1}, "AB", pixels, {.width = 1, .height = 3}, kBitmapPath);
    ASSERT_TRUE(font.is_valid());

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const graphics::TextRasterizer rasterizer;
    rasterizer.rasterize(font, "AB", solid_sampler(kForeground), {.x = 2, .y = 3}, framebuffer);

    expect_rgb_at(framebuffer, 2, 3, 0, 200, 0);
    expect_rgb_at(framebuffer, 3, 3, 9, 9, 9);
    expect_rgb_at(framebuffer, 4, 3, 0, 200, 0);
    expect_rgb_at(framebuffer, 2, 4, 9, 9, 9);
    expect_rgb_at(framebuffer, 4, 4, 9, 9, 9);
}

TEST(TextRasterizer, AdvancesUnknownCharactersAsBlankCharacters)
{
    const std::array<graphics::RgbColor, 10> pixels{
        kBitmapMark, kBlack, kBlack, kBitmapMark, kBitmapMark, kBitmapMark, kBlack, kBitmapMark, kBitmapMark, kBlack,
    };
    const graphics::Font font = make_lookup_font(pixels);
    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const graphics::TextRasterizer rasterizer;
    rasterizer.rasterize(font, "A?B", solid_sampler(kForeground), {}, framebuffer);

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

TEST(TextRasterizer, DrawsNothingForEmptyTextOrInvalidFont)
{
    const std::array<graphics::RgbColor, 1> pixels{kBitmapMark};
    const graphics::Font font = make_font({.width = 1, .height = 1}, "A", pixels, {.width = 1, .height = 1});
    const graphics::Font invalid_font{};

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const graphics::LogicalFramebuffer original = framebuffer;
    const graphics::TextRasterizer rasterizer;

    rasterizer.rasterize(font, "", solid_sampler(kForeground), {}, framebuffer);
    rasterizer.rasterize(invalid_font, "A", solid_sampler(kForeground), {}, framebuffer);
    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

TEST(TextRasterizer, ClipsEachCanvasEdge)
{
    const std::array<graphics::RgbColor, 4> pixels{kBitmapMark, kBlack, kBlack, kBitmapMark};
    const graphics::Font font =
        make_font({.width = 2, .height = 2}, "Q", pixels, {.width = 2, .height = 2}, kBitmapPath);
    ASSERT_TRUE(font.is_valid());
    const graphics::TextRasterizer rasterizer;

    {
        graphics::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        rasterizer.rasterize(font, "Q", solid_sampler(kForeground), {.x = -1, .y = 0}, framebuffer);
        expect_rgb_at(framebuffer, 0, 0, 9, 9, 9);
        expect_rgb_at(framebuffer, 0, 1, 0, 200, 0);
        expect_rgb_at(framebuffer, 1, 0, 9, 9, 9);
    }
    {
        graphics::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        rasterizer.rasterize(font, "Q", solid_sampler(kForeground), {.x = 0, .y = -1}, framebuffer);
        expect_rgb_at(framebuffer, 0, 0, 9, 9, 9);
        expect_rgb_at(framebuffer, 1, 0, 0, 200, 0);
        expect_rgb_at(framebuffer, 0, 1, 9, 9, 9);
    }
    {
        graphics::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        rasterizer.rasterize(font, "Q", solid_sampler(kForeground), {.x = 31, .y = 0}, framebuffer);
        expect_rgb_at(framebuffer, 31, 0, 0, 200, 0);
        expect_rgb_at(framebuffer, 31, 1, 9, 9, 9);
        expect_rgb_at(framebuffer, 30, 0, 9, 9, 9);
    }
    {
        graphics::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        rasterizer.rasterize(font, "Q", solid_sampler(kForeground), {.x = 0, .y = 7}, framebuffer);
        expect_rgb_at(framebuffer, 0, 7, 0, 200, 0);
        expect_rgb_at(framebuffer, 1, 7, 9, 9, 9);
        expect_rgb_at(framebuffer, 0, 6, 9, 9, 9);
    }
}

namespace {

[[nodiscard]] graphics::ColorSampler red_blue_sampler(float angle_degrees, graphics::Position origin,
                                                       graphics::Size size)
{
    const graphics::LinearGradientPaint gradient{
        .angle_degrees = angle_degrees,
        .color_stop_count = 2,
        .color_stops =
            {
                graphics::GradientColorStop{.offset = 0.0f, .color = {.red = 255, .green = 0, .blue = 0}},
                graphics::GradientColorStop{.offset = 1.0f, .color = {.red = 0, .green = 0, .blue = 255}},
            },
    };
    return graphics::ColorSampler{graphics::Paint{gradient}, origin, size};
}

TEST(TextRasterizer, SamplesGradientAtEachOpaqueGlyphPixel)
{
    const std::array<graphics::RgbColor, 4> pixels{
        kBitmapMark,
        kBitmapMark,
        kBlack,
        kBitmapMark,
    };
    const graphics::Font font =
        make_font({.width = 2, .height = 2}, "Q", pixels, {.width = 2, .height = 2}, kBitmapPath);
    ASSERT_TRUE(font.is_valid());

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const graphics::TextRasterizer rasterizer;
    const graphics::Position origin{.x = 4, .y = 1};
    const graphics::Size measured = graphics::measure_text(font, "Q");
    expect_size(measured, 2, 2);
    rasterizer.rasterize(font, "Q", red_blue_sampler(90.0f, origin, measured), origin, framebuffer);

    expect_rgb_at(framebuffer, 4, 1, 255, 0, 0);
    expect_rgb_at(framebuffer, 5, 1, 128, 0, 128);
    expect_rgb_at(framebuffer, 4, 2, 9, 9, 9);
    expect_rgb_at(framebuffer, 5, 2, 128, 0, 128);
    expect_rgb_at(framebuffer, 3, 1, 9, 9, 9);
    expect_rgb_at(framebuffer, 6, 1, 9, 9, 9);
    expect_rgb_at(framebuffer, 4, 0, 9, 9, 9);
    expect_rgb_at(framebuffer, 5, 3, 9, 9, 9);
}

TEST(TextRasterizer, GradientLeavesGapsAndClipsUsingCanvasPosition)
{
    const std::array<graphics::RgbColor, 3> pixels{
        kBitmapMark,
        kBlack,
        kBitmapMark,
    };
    const graphics::Font font =
        make_font({.width = 1, .height = 1}, "AB", pixels, {.width = 1, .height = 3}, kBitmapPath);
    ASSERT_TRUE(font.is_valid());
    const graphics::TextRasterizer rasterizer;

    {
        graphics::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        const graphics::Position origin{};
        const graphics::Size measured = graphics::measure_text(font, "AB");
        rasterizer.rasterize(font, "AB", red_blue_sampler(90.0f, origin, measured), origin, framebuffer);
        expect_rgb_at(framebuffer, 0, 0, 255, 0, 0);
        expect_rgb_at(framebuffer, 1, 0, 9, 9, 9);
        expect_rgb_at(framebuffer, 2, 0, 85, 0, 170);
    }
    {
        const std::array<graphics::RgbColor, 2> wide_pixels{kBitmapMark, kBitmapMark};
        const graphics::Font wide =
            make_font({.width = 2, .height = 1}, "Q", wide_pixels, {.width = 2, .height = 1}, kBitmapPath);
        ASSERT_TRUE(wide.is_valid());
        graphics::LogicalFramebuffer framebuffer;
        framebuffer.clear(kBackground);
        const graphics::Position origin{.x = -1, .y = 0};
        const graphics::Size measured = graphics::measure_text(wide, "Q");
        rasterizer.rasterize(wide, "Q", red_blue_sampler(90.0f, origin, measured), origin, framebuffer);
        expect_rgb_at(framebuffer, 0, 0, 128, 0, 128);
        expect_rgb_at(framebuffer, 1, 0, 9, 9, 9);
    }
}

}  // namespace
