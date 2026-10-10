#pragma once

#include "bitmap_file.hpp"
#include "geometry.hpp"
#include "logical_framebuffer.hpp"

namespace graphics {

class BitmapRasterizer final {
public:
    void rasterize(BitmapFile bitmap, Position origin_on_canvas, LogicalFramebuffer& framebuffer) const;
};

}  // namespace graphics
