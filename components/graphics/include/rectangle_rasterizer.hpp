#pragma once

#include "color.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace display {

class RectangleRasterizer final {
public:
    void rasterize(Size size, RgbColor color, Position canvas_origin,
                   LogicalFramebuffer& framebuffer) const;
};

}  // namespace display
