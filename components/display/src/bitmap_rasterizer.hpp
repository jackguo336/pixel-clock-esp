#pragma once

#include "display/bitmap_file.hpp"
#include "display/geometry.hpp"
#include "logical_framebuffer.hpp"

namespace display {

class BitmapRasterizer final {
public:
    void rasterize(BitmapFile bitmap, Position canvas_origin, LogicalFramebuffer& framebuffer) const;
};

}  // namespace display
