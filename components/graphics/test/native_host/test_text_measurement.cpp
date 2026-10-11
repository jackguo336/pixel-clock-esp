#include <array>
#include <cstdint>
#include <limits>
#include <string>

#include "gtest/gtest.h"
#include "text_measurement.hpp"
#include "utils.hpp"

using graphics_test::expect_size;
using graphics_test::make_font;
using graphics_test::make_lookup_font;

namespace {

constexpr graphics::RgbColor kBitmapMark{.red = 255, .green = 0, .blue = 0};
constexpr graphics::RgbColor kBlack{};

}  // namespace

TEST(TextMeasurement, MeasuresEmptyTextAndInvalidFontAsZero)
{
    const std::array<graphics::RgbColor, 1> pixels{kBitmapMark};
    const graphics::Font font = make_font({.width = 1, .height = 1}, "A", pixels, {.width = 1, .height = 1});
    const graphics::Font invalid_font{};

    expect_size(graphics::measure_text(font, ""), 0, 0);
    expect_size(graphics::measure_text(invalid_font, "A"), 0, 0);
}

TEST(TextMeasurement, IncludesOneGapPixelBetweenCharacters)
{
    const std::array<graphics::RgbColor, 3> pixels{
        kBitmapMark,
        kBlack,
        kBitmapMark,
    };
    const graphics::Font font =
        make_font({.width = 1, .height = 1}, "AB", pixels, {.width = 1, .height = 3}, graphics_test::kBitmapPath);
    ASSERT_TRUE(font.is_valid());

    expect_size(graphics::measure_text(font, "AB"), 3, 1);
}

TEST(TextMeasurement, CountsMissingGlyphsAsBlankCharacters)
{
    const std::array<graphics::RgbColor, 10> pixels{
        kBitmapMark, kBlack, kBlack, kBitmapMark, kBitmapMark, kBitmapMark, kBlack, kBitmapMark, kBitmapMark, kBlack,
    };
    const graphics::Font font = make_lookup_font(pixels);
    ASSERT_TRUE(font.is_valid());

    expect_size(graphics::measure_text(font, "A?B"), 8, 2);
}

TEST(TextMeasurement, ClampsWidthToUint16Max)
{
    constexpr uint16_t kCharacterWidth = 255;
    const std::array<graphics::RgbColor, kCharacterWidth> pixels{};
    const graphics::Font font =
        make_font({.width = kCharacterWidth, .height = 1}, "A", pixels, {.width = kCharacterWidth, .height = 1});
    ASSERT_TRUE(font.is_valid());

    const std::string text(257, 'A');
    expect_size(graphics::measure_text(font, text), std::numeric_limits<uint16_t>::max(), 1);
}
