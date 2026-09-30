#pragma once

#include <string_view>

#include "color.hpp"
#include "font.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace display {

class TextRasterizer final {
public:
    [[nodiscard]] Size rasterize(const Font& font, std::string_view text, RgbColor foreground,
                                 Position canvas_origin, LogicalFramebuffer& framebuffer) const;
};

}  // namespace display
