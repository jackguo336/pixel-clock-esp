#pragma once

#include "element_tree.hpp"
#include "logical_framebuffer.hpp"

namespace graphics {

class ElementTreeRenderer final {
public:
    void render(const ElementTree& tree, LogicalFramebuffer& framebuffer) const;
};

}  // namespace graphics
