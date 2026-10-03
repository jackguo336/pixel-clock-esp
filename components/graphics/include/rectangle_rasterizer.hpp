#pragma once

#include "color_sampler.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace graphics {

class RectangleRasterizer final {
public:
    void rasterize(Size size, const ColorSampler& color_sampler, Position canvas_origin,
                   LogicalFramebuffer& framebuffer) const;
};

}  // namespace graphics
