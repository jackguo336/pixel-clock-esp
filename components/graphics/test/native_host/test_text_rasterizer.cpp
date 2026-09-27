#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "font.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"
#include "text_rasterizer.hpp"

namespace {

constexpr display::RgbColor kBackground{.red = 9, .green = 9, .blue = 9};
constexpr display::RgbColor kForeground{.red = 0, .green = 200, .blue = 0};
constexpr display::RgbColor kAtlasMark{.red = 255, .green = 0, .blue = 0};
constexpr display::RgbColor kBlack{};
constexpr display::Size kWideGlyph{.width = 2, .height = 2};
constexpr const char* kAtlasPath = "/assets/fonts/atlas.bmp";

void expect_rgb_at(const display::LogicalFramebuffer& framebuffer, int32_t x, int32_t y, uint8_t red,
                   uint8_t green, uint8_t blue)
{
    const display::RgbColor* pixel = framebuffer.pixel_at(x, y);
    ASSERT_NE(pixel, nullptr);
    EXPECT_EQ(pixel->red, red);
    EXPECT_EQ(pixel->green, green);
    EXPECT_EQ(pixel->blue, blue);
}

void expect_size(display::Size size, uint16_t width, uint16_t height)
{
    EXPECT_EQ(size.width, width);
    EXPECT_EQ(size.height, height);
}

[[nodiscard]] bool framebuffers_equal(const display::LogicalFramebuffer& lhs,
                                      const display::LogicalFramebuffer& rhs)
{
    const auto left = lhs.pixels();
    const auto right = rhs.pixels();
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i].red != right[i].red || left[i].green != right[i].green || left[i].blue != right[i].blue) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] display::Font make_font(display::Size character_size, std::string_view lookup,
                                      std::span<const display::RgbColor> pixels, display::Size bitmap_size,
                                      const char* bitmap_path = nullptr)
{
    return display::Font{
        .config = {
            .character_size = character_size,
            .character_lookup = lookup,
            .bitmap_path = bitmap_path,
        },
        .bitmap = {
            .size = bitmap_size,
            .pixels = pixels,
        },
    };
}

// Lookup "BA": 'B' is row 0 and 'A' is row 1, with a non-black separator between them.
// B: mark, black / black, mark
// separator: mark, mark
// A: black, mark / mark, black
[[nodiscard]] display::Font make_lookup_font(std::span<const display::RgbColor> pixels)
{
    return make_font(kWideGlyph, "BA", pixels, {.width = 2, .height = 5}, kAtlasPath);
}

}  // namespace

TEST(Font, AcceptsOneCharacterWideStackedAtlas)
{
    const std::array<display::RgbColor, 1> single_glyph{kAtlasMark};
    const display::Font single_glyph_font = make_font(
        {.width = 1, .height = 1}, "A", single_glyph, {.width = 1, .height = 1});
    EXPECT_TRUE(single_glyph_font.is_valid());
    EXPECT_EQ(single_glyph_font.config.bitmap_path, nullptr);

    const std::array<display::RgbColor, 10> stacked_atlas{
        kAtlasMark, kBlack, kBlack, kAtlasMark, kAtlasMark, kAtlasMark, kBlack, kAtlasMark, kAtlasMark, kBlack,
    };
    const display::Font stacked_font = make_lookup_font(stacked_atlas);
    EXPECT_TRUE(stacked_font.is_valid());
    EXPECT_STREQ(stacked_font.config.bitmap_path, kAtlasPath);
}

TEST(Font, RejectsEmptyAndMismatchedAtlases)
{
    const std::array<display::RgbColor, 1> one_pixel{kAtlasMark};
    const std::array<display::RgbColor, 2> two_pixels{kAtlasMark, kBlack};
    const display::Font empty_font{};
    const display::Font empty_lookup =
        make_font({.width = 1, .height = 1}, "", one_pixel, {.width = 1, .height = 1});
    const display::Font zero_character_size = make_font({}, "A", one_pixel, {.width = 1, .height = 1});
    const display::Font wider_than_one_character =
        make_font({.width = 1, .height = 1}, "A", two_pixels, {.width = 2, .height = 1});
    const display::Font missing_separator =
        make_font({.width = 1, .height = 1}, "AB", two_pixels, {.width = 1, .height = 2});
    const display::Font extra_separator_row =
        make_font({.width = 1, .height = 1}, "A", two_pixels, {.width = 1, .height = 2});
    const display::Font short_pixel_span =
        make_font({.width = 2, .height = 1}, "A", one_pixel, {.width = 2, .height = 1});

    EXPECT_FALSE(empty_font.is_valid());
    EXPECT_FALSE(empty_lookup.is_valid());
    EXPECT_FALSE(zero_character_size.is_valid());
    EXPECT_FALSE(wider_than_one_character.is_valid());
    EXPECT_FALSE(missing_separator.is_valid());
    EXPECT_FALSE(extra_separator_row.is_valid());
    EXPECT_FALSE(short_pixel_span.is_valid());

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const display::LogicalFramebuffer original = framebuffer;
    const display::TextRasterizer rasterizer;
    expect_size(rasterizer.rasterize(empty_font, "A", kForeground, {}, framebuffer), 0, 0);
    expect_size(rasterizer.rasterize(missing_separator, "AB", kForeground, {}, framebuffer), 0, 0);
    expect_size(rasterizer.rasterize(wider_than_one_character, "A", kForeground, {}, framebuffer), 0, 0);
    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

TEST(TextRasterizer, LooksUpGlyphRowsAndSkipsSourceSeparators)
{
    const std::array<display::RgbColor, 10> pixels{
        kAtlasMark, kBlack, kBlack, kAtlasMark, kAtlasMark, kAtlasMark, kBlack, kAtlasMark, kAtlasMark, kBlack,
    };
    const display::Font font = make_lookup_font(pixels);
    ASSERT_TRUE(font.is_valid());

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const display::TextRasterizer rasterizer;

    expect_size(rasterizer.rasterize(font, "B", kForeground, {}, framebuffer), 2, 2);
    expect_rgb_at(framebuffer, 0, 0, 0, 200, 0);
    expect_rgb_at(framebuffer, 1, 0, 9, 9, 9);
    expect_rgb_at(framebuffer, 0, 1, 9, 9, 9);
    expect_rgb_at(framebuffer, 1, 1, 0, 200, 0);
    expect_rgb_at(framebuffer, 0, 2, 9, 9, 9);
    expect_rgb_at(framebuffer, 1, 2, 9, 9, 9);

    framebuffer.clear(kBackground);
    expect_size(rasterizer.rasterize(font, "A", kForeground, {}, framebuffer), 2, 2);
    expect_rgb_at(framebuffer, 0, 0, 9, 9, 9);
    expect_rgb_at(framebuffer, 1, 0, 0, 200, 0);
    expect_rgb_at(framebuffer, 0, 1, 0, 200, 0);
    expect_rgb_at(framebuffer, 1, 1, 9, 9, 9);
    expect_rgb_at(framebuffer, 0, 2, 9, 9, 9);
    expect_rgb_at(framebuffer, 1, 2, 9, 9, 9);
}

TEST(TextRasterizer, PaintsNonBlackAtlasPixelsWithSolidColorAndLeavesBlackUnchanged)
{
    const std::array<display::RgbColor, 3> pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{},
        display::RgbColor{.red = 0, .green = 0, .blue = 1},
    };
    const display::Font font =
        make_font({.width = 3, .height = 1}, "M", pixels, {.width = 3, .height = 1}, kAtlasPath);
    ASSERT_TRUE(font.is_valid());

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 40, .green = 50, .blue = 60});
    const display::TextRasterizer rasterizer;
    const display::Size size = rasterizer.rasterize(
        font, "M", display::RgbColor{.red = 10, .green = 20, .blue = 30}, {.x = 1, .y = 1}, framebuffer);

    expect_size(size, 3, 1);
    expect_rgb_at(framebuffer, 1, 1, 10, 20, 30);
    expect_rgb_at(framebuffer, 2, 1, 40, 50, 60);
    expect_rgb_at(framebuffer, 3, 1, 10, 20, 30);
    expect_rgb_at(framebuffer, 0, 1, 40, 50, 60);
    expect_rgb_at(framebuffer, 4, 1, 40, 50, 60);
}

TEST(TextRasterizer, LeavesOneTransparentPixelBetweenCharacters)
{
    const std::array<display::RgbColor, 3> pixels{
        display::RgbColor{.red = 255, .green = 255, .blue = 255},
        kAtlasMark,
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const display::Font font =
        make_font({.width = 1, .height = 1}, "AB", pixels, {.width = 1, .height = 3}, kAtlasPath);
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

TEST(TextRasterizer, AdvancesUnknownCharactersAsBlankGlyphs)
{
    const std::array<display::RgbColor, 10> pixels{
        kAtlasMark, kBlack, kBlack, kAtlasMark, kAtlasMark, kAtlasMark, kBlack, kAtlasMark, kAtlasMark, kBlack,
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
    const std::array<display::RgbColor, 1> pixels{kAtlasMark};
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
    const std::array<display::RgbColor, 4> pixels{kAtlasMark, kBlack, kBlack, kAtlasMark};
    const display::Font font =
        make_font({.width = 2, .height = 2}, "Q", pixels, {.width = 2, .height = 2}, kAtlasPath);
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

TEST(TextRasterizer, ReturnsUnclippedSizeWhenTextIsFullyOffCanvas)
{
    const std::array<display::RgbColor, 4> pixels{kAtlasMark, kAtlasMark, kAtlasMark, kAtlasMark};
    const display::Font font = make_font({.width = 2, .height = 2}, "Q", pixels, {.width = 2, .height = 2});
    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(kBackground);
    const display::LogicalFramebuffer original = framebuffer;
    const display::TextRasterizer rasterizer;

    expect_size(rasterizer.rasterize(font, "Q", kForeground, {.x = -2, .y = 0}, framebuffer), 2, 2);
    expect_size(rasterizer.rasterize(font, "Q", kForeground, {.x = 32, .y = 0}, framebuffer), 2, 2);
    expect_size(rasterizer.rasterize(font, "Q", kForeground, {.x = 0, .y = -2}, framebuffer), 2, 2);
    expect_size(rasterizer.rasterize(font, "Q", kForeground, {.x = 0, .y = 8}, framebuffer), 2, 2);
    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}
