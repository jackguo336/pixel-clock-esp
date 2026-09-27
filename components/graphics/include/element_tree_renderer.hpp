#pragma once

#include <optional>

#include "bitmap_rasterizer.hpp"
#include "element_tree.hpp"
#include "logical_framebuffer.hpp"
#include "text_rasterizer.hpp"

namespace display {

class ElementTreeRenderer final {
public:
    void render(const ElementTree& tree, LogicalFramebuffer& framebuffer) const;

private:
    [[nodiscard]] Size render_node(const ElementTree& tree, ElementNodeIndex index, Position canvas_origin,
                                   LogicalFramebuffer& framebuffer) const;

    [[nodiscard]] Size render_children(const ElementTree& tree, std::optional<ElementNodeIndex> first_child,
                                       Position container_origin, const ContainerElementPayload& container,
                                       LogicalFramebuffer& framebuffer) const;

    BitmapRasterizer bitmap_rasterizer_{};
    TextRasterizer text_rasterizer_{};
};

}  // namespace display
