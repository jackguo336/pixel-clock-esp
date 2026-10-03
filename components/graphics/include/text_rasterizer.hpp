#pragma once

#include <string_view>

#include "color_sampler.hpp"
#include "font.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace graphics {

class TextRasterizer final {
public:
    [[nodiscard]] Size measure(const Font& font, std::string_view text) const;

    [[nodiscard]] Size rasterize(const Font& font, std::string_view text, const ColorSampler& foreground,
                                 Position canvas_origin, LogicalFramebuffer& framebuffer) const;
};

}  // namespace graphics
