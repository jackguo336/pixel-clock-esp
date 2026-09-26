#pragma once

#include "bitmap_rasterizer.hpp"
#include "element_tree.hpp"
#include "logical_framebuffer.hpp"
#include "text_rasterizer.hpp"

namespace display {

class ElementTreeRenderer final {
public:
    void render(const ElementTree& tree, LogicalFramebuffer& framebuffer) const;

private:
    BitmapRasterizer bitmap_rasterizer_{};
    TextRasterizer text_rasterizer_{};
};

}  // namespace display
