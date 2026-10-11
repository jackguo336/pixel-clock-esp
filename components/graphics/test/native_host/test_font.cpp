#include <array>

#include "font.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"
#include "text_measurement.hpp"
#include "text_rasterizer.hpp"
#include "utils.hpp"

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

TEST(FontConfig, ConfiguresStackedBitmapStorage)
{
    constexpr graphics::FontConfig config =
        graphics::configure_font<2, 3>("/assets/fonts/bitmap.bmp", "ABC");

    static_assert(config.pixel_count() == 22);
    expect_size(config.bitmap_size(), 2, 11);
    EXPECT_EQ(config.character_size.width, 2);
    EXPECT_EQ(config.character_size.height, 3);
    EXPECT_EQ(config.character_lookup, "ABC");
    EXPECT_STREQ(config.bitmap_path, "/assets/fonts/bitmap.bmp");
}

TEST(FontConfig, ReturnsEmptyStorageDimensionsForInvalidConfiguration)
{
    constexpr graphics::FontConfig empty_lookup =
        graphics::configure_font<2, 3>("/assets/fonts/bitmap.bmp", "");
    constexpr graphics::FontConfig overflowing_height =
        graphics::configure_font<1, 32768>("/assets/fonts/bitmap.bmp", "AB");

    expect_size(empty_lookup.bitmap_size(), 0, 0);
    EXPECT_EQ(empty_lookup.pixel_count(), 0);
    expect_size(overflowing_height.bitmap_size(), 0, 0);
    EXPECT_EQ(overflowing_height.pixel_count(), 0);
}

TEST(Font, AcceptsOneCharacterWideStackedBitmap)
{
    const std::array<graphics::RgbColor, 1> single_character{kBitmapMark};
    const graphics::Font single_character_font = make_font(
        {.width = 1, .height = 1}, "A", single_character, {.width = 1, .height = 1});
    EXPECT_TRUE(single_character_font.is_valid());
    EXPECT_EQ(single_character_font.config.bitmap_path, nullptr);

    const std::array<graphics::RgbColor, 10> stacked_bitmap{
        kBitmapMark, kBlack, kBlack, kBitmapMark, kBitmapMark, kBitmapMark, kBlack, kBitmapMark, kBitmapMark, kBlack,
    };
    const graphics::Font stacked_font = make_lookup_font(stacked_bitmap);
    EXPECT_TRUE(stacked_font.is_valid());
    EXPECT_STREQ(stacked_font.config.bitmap_path, kBitmapPath);
}

TEST(Font, RejectsEmptyAndMismatchedBitmaps)
{
    const std::array<graphics::RgbColor, 1> one_pixel{kBitmapMark};
    const std::array<graphics::RgbColor, 2> two_pixels{kBitmapMark, kBlack};
    const graphics::Font empty_font{};
    const graphics::Font empty_lookup =
        make_font({.width = 1, .height = 1}, "", one_pixel, {.width = 1, .height = 1});
    const graphics::Font zero_character_size = make_font({}, "A", one_pixel, {.width = 1, .height = 1});
    const graphics::Font wider_than_one_character =
        make_font({.width = 1, .height = 1}, "A", two_pixels, {.width = 2, .height = 1});
    const graphics::Font missing_separator =
        make_font({.width = 1, .height = 1}, "AB", two_pixels, {.width = 1, .height = 2});
    const graphics::Font extra_separator_row =
        make_font({.width = 1, .height = 1}, "A", two_pixels, {.width = 1, .height = 2});
    const graphics::Font short_pixel_span =
        make_font({.width = 2, .height = 1}, "A", one_pixel, {.width = 2, .height = 1});

    EXPECT_FALSE(empty_font.is_valid());
    EXPECT_FALSE(empty_lookup.is_valid());
    EXPECT_FALSE(zero_character_size.is_valid());
    EXPECT_FALSE(wider_than_one_character.is_valid());
    EXPECT_FALSE(missing_separator.is_valid());
    EXPECT_FALSE(extra_separator_row.is_valid());
    EXPECT_FALSE(short_pixel_span.is_valid());

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const graphics::LogicalFramebuffer original = framebuffer;
    const graphics::TextRasterizer rasterizer;
    expect_size(graphics::measure_text(empty_font, "A"), 0, 0);
    expect_size(graphics::measure_text(missing_separator, "AB"), 0, 0);
    expect_size(graphics::measure_text(wider_than_one_character, "A"), 0, 0);
    rasterizer.rasterize(empty_font, "A", solid_sampler(kForeground), {}, framebuffer);
    rasterizer.rasterize(missing_separator, "AB", solid_sampler(kForeground), {}, framebuffer);
    rasterizer.rasterize(wider_than_one_character, "A", solid_sampler(kForeground), {}, framebuffer);
    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}
