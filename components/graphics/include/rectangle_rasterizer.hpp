#pragma once

#include "color.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace graphics {

class RectangleRasterizer final {
public:
    void rasterize(Size size, RgbColor color, Position canvas_origin,
                   LogicalFramebuffer& framebuffer) const;
};

}  // namespace graphics
