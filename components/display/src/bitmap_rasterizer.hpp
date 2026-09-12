#pragma once

#include "display/bitmap.hpp"
#include "display/elements.hpp"
#include "logical_framebuffer.hpp"

namespace display {

class BitmapRasterizer final {
public:
    void rasterize(BitmapView bitmap, Position canvas_origin, LogicalFramebuffer& framebuffer) const;
};

}  // namespace display
