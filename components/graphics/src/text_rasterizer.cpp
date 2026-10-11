#include "text_rasterizer.hpp"

#include <cstddef>
#include <cstdint>

#include "text_measurement.hpp"

namespace graphics {
namespace {

[[nodiscard]] bool is_foreground_bitmap_pixel(RgbColor color)
{
    return color.red != 0 || color.green != 0 || color.blue != 0;
}

[[nodiscard]] bool is_inside_framebuffer(int32_t x, int32_t y)
{
    return x >= 0 && y >= 0 && x < LogicalFramebuffer::kWidth && y < LogicalFramebuffer::kHeight;
}

void draw_character(const Font& font, std::size_t character_index, int32_t origin_x, int32_t origin_y,
                    const ColorSampler& color_sampler, LogicalFramebuffer& framebuffer)
{
    const int32_t character_width = static_cast<int32_t>(font.config.character_size.width);
    const int32_t character_height = static_cast<int32_t>(font.config.character_size.height);
    const std::size_t bitmap_width = font.bitmap.size.width;
    const std::size_t character_bitmap_top_row =
        character_index * (static_cast<std::size_t>(font.config.character_size.height)
                           + kBitmapCharacterSeparatorPixels);

    for (int32_t row = 0; row < character_height; ++row) {
        const int32_t canvas_y = origin_y + row;
        const std::size_t row_offset =
            (character_bitmap_top_row + static_cast<std::size_t>(row)) * bitmap_width;
        for (int32_t column = 0; column < character_width; ++column) {
            const RgbColor bitmap_pixel = font.bitmap.pixels[row_offset + static_cast<std::size_t>(column)];
            if (!is_foreground_bitmap_pixel(bitmap_pixel)) {
                continue;
            }
            const int32_t canvas_x = origin_x + column;
            if (!is_inside_framebuffer(canvas_x, canvas_y)) {
                continue;
            }
            const RgbColor color = color_sampler.sample(Position{
                .x = static_cast<int16_t>(canvas_x),
                .y = static_cast<int16_t>(canvas_y),
            });
            static_cast<void>(framebuffer.set_pixel(canvas_x, canvas_y, color));
        }
    }
}

void draw_text(const Font& font, std::string_view text, const ColorSampler& color_sampler, Position origin_on_canvas,
               LogicalFramebuffer& framebuffer)
{
    const int32_t character_width_plus_gap =
        static_cast<int32_t>(font.config.character_size.width) + kInterCharacterGapPixels;
    for (std::size_t text_index = 0; text_index < text.size(); ++text_index) {
        const char character = text[text_index];
        const std::size_t character_index = font.config.character_lookup.find(character);
        if (character_index == std::string_view::npos) {
            continue;
        }
        const int32_t character_position_x =
            origin_on_canvas.x + static_cast<int32_t>(text_index) * character_width_plus_gap;
        draw_character(font, character_index, character_position_x, origin_on_canvas.y, color_sampler, framebuffer);
    }
}

}  // namespace

void TextRasterizer::rasterize(const Font& font, std::string_view text, const ColorSampler& color_sampler,
                               Position origin_on_canvas, LogicalFramebuffer& framebuffer) const
{
    if (!font.is_valid() || text.empty()) {
        return;
    }
    draw_text(font, text, color_sampler, origin_on_canvas, framebuffer);
}

}  // namespace graphics
