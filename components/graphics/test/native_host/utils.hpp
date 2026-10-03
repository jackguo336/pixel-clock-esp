#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "font.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"

namespace graphics_test {

inline constexpr const char* kBitmapPath = "/assets/fonts/bitmap.bmp";

inline void expect_size(graphics::Size size, uint16_t width, uint16_t height)
{
    EXPECT_EQ(size.width, width);
    EXPECT_EQ(size.height, height);
}

inline void expect_rgb(const graphics::RgbColor& actual, uint8_t red, uint8_t green, uint8_t blue)
{
    EXPECT_EQ(actual.red, red);
    EXPECT_EQ(actual.green, green);
    EXPECT_EQ(actual.blue, blue);
}

inline void expect_rgb_at(const graphics::LogicalFramebuffer& framebuffer, int32_t x, int32_t y, uint8_t red,
                          uint8_t green, uint8_t blue)
{
    const graphics::RgbColor* pixel = framebuffer.pixel_at(x, y);
    ASSERT_NE(pixel, nullptr);
    expect_rgb(*pixel, red, green, blue);
}

[[nodiscard]] inline bool framebuffers_equal(const graphics::LogicalFramebuffer& lhs,
                                             const graphics::LogicalFramebuffer& rhs)
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

[[nodiscard]] inline graphics::Font make_font(graphics::Size character_size, std::string_view lookup,
                                             std::span<const graphics::RgbColor> pixels, graphics::Size bitmap_size,
                                             const char* bitmap_path = nullptr)
{
    return graphics::Font{
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
[[nodiscard]] inline graphics::Font make_lookup_font(std::span<const graphics::RgbColor> pixels)
{
    constexpr graphics::Size kWideCharacter{.width = 2, .height = 2};
    return make_font(kWideCharacter, "BA", pixels, {.width = 2, .height = 5}, kBitmapPath);
}

}  // namespace graphics_test
