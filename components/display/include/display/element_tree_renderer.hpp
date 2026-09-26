#pragma once

#include "display/bitmap_rasterizer.hpp"
#include "display/element_tree.hpp"
#include "display/logical_framebuffer.hpp"

namespace display {

class ElementTreeRenderer final {
public:
    void render(const ElementTree& tree, LogicalFramebuffer& framebuffer) const;

private:
    BitmapRasterizer bitmap_rasterizer_{};
};

}  // namespace display
