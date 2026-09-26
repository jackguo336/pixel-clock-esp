#pragma once

#include <string_view>

#include "color.hpp"
#include "font.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace display {

class TextRasterizer final {
public:
    // Invalid fonts and empty text measure as zero. Does not draw.
    [[nodiscard]] Size measure(const Font& font, std::string_view text) const;

    // Paints non-black atlas pixels with `foreground` and returns measure().
    // Unknown characters advance one blank glyph. Black atlas pixels are left unchanged.
    [[nodiscard]] Size rasterize(const Font& font, std::string_view text, RgbColor foreground,
                                 Position canvas_origin, LogicalFramebuffer& framebuffer) const;
};

}  // namespace display
