#pragma once

#include <string_view>

#include "color.hpp"
#include "font.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace display {

class TextRasterizer final {
public:
    // Paints non-black bitmap pixels with `foreground` and returns the logical text size.
    // Unknown characters advance one blank character. Black bitmap pixels are left unchanged.
    // Invalid fonts and empty text return zero and draw nothing.
    [[nodiscard]] Size rasterize(const Font& font, std::string_view text, RgbColor foreground,
                                 Position canvas_origin, LogicalFramebuffer& framebuffer) const;
};

}  // namespace display
