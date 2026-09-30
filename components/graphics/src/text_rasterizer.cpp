#include "text_rasterizer.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace display {
namespace {

constexpr int32_t kInterCharacterGapPixels = 1;

[[nodiscard]] bool is_foreground_bitmap_pixel(RgbColor color)
{
    return color.red != 0 || color.green != 0 || color.blue != 0;
}

[[nodiscard]] Size measured_text_size(const Font& font, std::string_view text)
{
    if (!font.is_valid() || text.empty()) {
        return {};
    }

    const uint64_t character_count = text.size();
    const uint64_t character_width = font.config.character_size.width;
    const uint64_t width = character_count * character_width
        + (character_count - 1) * static_cast<uint64_t>(kInterCharacterGapPixels);
    constexpr uint64_t kMaxExtent = std::numeric_limits<uint16_t>::max();
    return Size{
        .width = static_cast<uint16_t>(width > kMaxExtent ? kMaxExtent : width),
        .height = font.config.character_size.height,
    };
}

void draw_character(const Font& font, std::size_t character_index, int32_t origin_x, int32_t origin_y,
                    RgbColor foreground, LogicalFramebuffer& framebuffer)
{
    const int32_t character_width = static_cast<int32_t>(font.config.character_size.width);
    const int32_t character_height = static_cast<int32_t>(font.config.character_size.height);
    const std::size_t bitmap_width = font.bitmap.size.width;
    const std::size_t character_top =
        character_index * (static_cast<std::size_t>(font.config.character_size.height)
                           + kBitmapCharacterSeparatorPixels);

    for (int32_t row = 0; row < character_height; ++row) {
        const int32_t canvas_y = origin_y + row;
        const std::size_t row_offset = (character_top + static_cast<std::size_t>(row)) * bitmap_width;
        for (int32_t column = 0; column < character_width; ++column) {
            const RgbColor bitmap_pixel = font.bitmap.pixels[row_offset + static_cast<std::size_t>(column)];
            if (!is_foreground_bitmap_pixel(bitmap_pixel)) {
                continue;
            }
            static_cast<void>(framebuffer.set_pixel(origin_x + column, canvas_y, foreground));
        }
    }
}

void draw_text(const Font& font, std::string_view text, RgbColor foreground, Position canvas_origin,
               LogicalFramebuffer& framebuffer)
{
    const int32_t pitch =
        static_cast<int32_t>(font.config.character_size.width) + kInterCharacterGapPixels;
    for (std::size_t text_index = 0; text_index < text.size(); ++text_index) {
        const char character = text[text_index];
        const std::size_t character_index = font.config.character_lookup.find(character);
        if (character_index == std::string_view::npos) {
            continue;
        }
        const int32_t character_x = canvas_origin.x + static_cast<int32_t>(text_index) * pitch;
        draw_character(font, character_index, character_x, canvas_origin.y, foreground, framebuffer);
    }
}

}  // namespace

Size TextRasterizer::rasterize(const Font& font, std::string_view text, RgbColor foreground,
                               Position canvas_origin, LogicalFramebuffer& framebuffer) const
{
    const Size size = measured_text_size(font, text);
    if (size.width == 0 || size.height == 0) {
        return size;
    }
    draw_text(font, text, foreground, canvas_origin, framebuffer);
    return size;
}

}  // namespace display
