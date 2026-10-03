#pragma once

#include "bitmap_file.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace graphics {

class BitmapRasterizer final {
public:
    void rasterize(BitmapFile bitmap, Position canvas_origin, LogicalFramebuffer& framebuffer) const;
};

}  // namespace graphics
