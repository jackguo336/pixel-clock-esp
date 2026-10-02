#pragma once

#include <optional>

#include "bitmap_rasterizer.hpp"
#include "element_tree.hpp"
#include "logical_framebuffer.hpp"
#include "rectangle_rasterizer.hpp"
#include "text_rasterizer.hpp"

namespace display {

class ElementTreeRenderer final {
public:
    void render(const ElementTree& tree, LogicalFramebuffer& framebuffer) const;

private:
    // Paint in effect for a node, plus the canvas origin of the element that
    // defined it. A descendant with no paint reuses both, so a later gradient
    // rasterizer can interpret start and end in that ancestor's coordinates.
    struct ResolvedPaint {
        Paint paint{};
        Position defining_origin{};
    };

    [[nodiscard]] static std::optional<ResolvedPaint> resolve_paint(
        const std::optional<Paint>& element_paint, Position element_origin,
        const std::optional<ResolvedPaint>& inherited_paint);

    [[nodiscard]] static const SolidPaint* as_solid_paint(const std::optional<ResolvedPaint>& resolved_paint);

    [[nodiscard]] Size render_node(const ElementTree& tree, ElementNodeIndex index, Position canvas_origin,
                                   const std::optional<ResolvedPaint>& inherited_paint,
                                   LogicalFramebuffer& framebuffer) const;

    [[nodiscard]] Size render_children(const ElementTree& tree, std::optional<ElementNodeIndex> first_child,
                                       Position container_origin, const ContainerElementPayload& container,
                                       const std::optional<ResolvedPaint>& inherited_paint,
                                       LogicalFramebuffer& framebuffer) const;

    BitmapRasterizer bitmap_rasterizer_{};
    TextRasterizer text_rasterizer_{};
    RectangleRasterizer rectangle_rasterizer_{};
};

}  // namespace display
