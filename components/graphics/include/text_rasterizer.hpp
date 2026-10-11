#pragma once

#include <string_view>

#include "color_sampler.hpp"
#include "font.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace graphics {

class TextRasterizer final {
public:
    void rasterize(const Font& font, std::string_view text, const ColorSampler& color_sampler, Position origin_on_canvas,
                   LogicalFramebuffer& framebuffer) const;
};

}  // namespace graphics
