#include "element_tree_renderer.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <variant>

#include "color.hpp"
#include "elements.hpp"
#include "geometry.hpp"

namespace display {
namespace {

[[nodiscard]] Position element_origin_on_canvas(Position parent_origin, Position element_position)
{
    return Position{
        .x = static_cast<int16_t>(parent_origin.x + element_position.x),
        .y = static_cast<int16_t>(parent_origin.y + element_position.y),
    };
}

[[nodiscard]] Position child_relative_position(const Element& element, const ContainerElementPayload& container,
                                               bool has_previous_sibling, Position previous_position,
                                               Size previous_size)
{
    if (container.layout_system != LayoutSystem::Stacked) {
        return element.position;
    }
    if (!has_previous_sibling) {
        return Position{};
    }

    const int32_t previous_x = previous_position.x;
    const int32_t previous_y = previous_position.y;
    const int32_t previous_width = static_cast<int32_t>(previous_size.width);
    const int32_t previous_height = static_cast<int32_t>(previous_size.height);
    switch (container.layout_direction) {
    case StackDirection::LeftToRight:
        return Position{
            .x = static_cast<int16_t>(previous_x + previous_width),
            .y = previous_position.y,
        };
    case StackDirection::TopToBottom:
        return Position{
            .x = previous_position.x,
            .y = static_cast<int16_t>(previous_y + previous_height),
        };
    }
    return previous_position;
}

[[nodiscard]] uint16_t clamp_int32_to_uint16(int32_t far_edge)
{
    constexpr int32_t kMaxExtent = std::numeric_limits<uint16_t>::max();
    return static_cast<uint16_t>(std::clamp(far_edge, int32_t{0}, kMaxExtent));
}

}  // namespace

std::optional<ElementTreeRenderer::ResolvedPaint> ElementTreeRenderer::resolve_paint(
    const std::optional<Paint>& element_paint, Position element_origin,
    const std::optional<ResolvedPaint>& inherited_paint)
{
    if (!element_paint.has_value()) {
        return inherited_paint;
    }
    return ResolvedPaint{
        .paint = *element_paint,
        .defining_origin = element_origin,
    };
}

const SolidPaint* ElementTreeRenderer::as_solid_paint(const std::optional<ResolvedPaint>& resolved_paint)
{
    if (!resolved_paint.has_value()) {
        return nullptr;
    }
    return std::get_if<SolidPaint>(&resolved_paint->paint);
}

Size ElementTreeRenderer::render_children(const ElementTree& tree, std::optional<ElementNodeIndex> first_child,
                                          Position container_origin, const ContainerElementPayload& container,
                                          const std::optional<ResolvedPaint>& inherited_paint,
                                          LogicalFramebuffer& framebuffer) const
{
    int32_t max_right = 0;
    int32_t max_bottom = 0;
    Position previous_position{};
    Size previous_size{};
    bool has_previous_sibling = false;

    std::optional<ElementNodeIndex> child_index = first_child;
    while (child_index.has_value()) {
        const ElementTreeNode& child = tree.nodes[*child_index];
        const Position relative_position = child_relative_position(
            child.element, container, has_previous_sibling, previous_position, previous_size);
        const Position child_origin = element_origin_on_canvas(container_origin, relative_position);
        const Size child_size = render_node(tree, *child_index, child_origin, inherited_paint, framebuffer);

        const int32_t right = static_cast<int32_t>(relative_position.x) + static_cast<int32_t>(child_size.width);
        const int32_t bottom =
            static_cast<int32_t>(relative_position.y) + static_cast<int32_t>(child_size.height);
        if (right > max_right) {
            max_right = right;
        }
        if (bottom > max_bottom) {
            max_bottom = bottom;
        }

        previous_position = relative_position;
        previous_size = child_size;
        has_previous_sibling = true;
        child_index = child.next_sibling;
    }

    return Size{
        .width = clamp_int32_to_uint16(max_right),
        .height = clamp_int32_to_uint16(max_bottom),
    };
}

Size ElementTreeRenderer::render_node(const ElementTree& tree, ElementNodeIndex index, Position canvas_origin,
                                      const std::optional<ResolvedPaint>& inherited_paint,
                                      LogicalFramebuffer& framebuffer) const
{
    const ElementTreeNode& node = tree.nodes[index];
    const std::optional<ResolvedPaint> resolved_paint =
        resolve_paint(node.element.paint, canvas_origin, inherited_paint);

    struct ElementPayloadVisitor {
        const ElementTreeRenderer& renderer;
        const ElementTree& tree;
        const ElementTreeNode& node;
        Position element_origin;
        const std::optional<ResolvedPaint>& resolved_paint;
        LogicalFramebuffer& framebuffer;

        Size operator()(const ContainerElementPayload& payload) const
        {
            return renderer.render_children(tree, node.first_child, element_origin, payload, resolved_paint,
                                            framebuffer);
        }

        Size operator()(const BitmapElementPayload& payload) const
        {
            if (payload.bitmap == nullptr) {
                return Size{};
            }
            renderer.bitmap_rasterizer_.rasterize(*payload.bitmap, element_origin, framebuffer);
            return payload.bitmap->size;
        }

        Size operator()(const TextElementPayload& payload) const
        {
            if (payload.font == nullptr || !payload.font->is_valid()) {
                return Size{};
            }
            if (const SolidPaint* solid_paint = ElementTreeRenderer::as_solid_paint(resolved_paint)) {
                return renderer.text_rasterizer_.rasterize(*payload.font, payload.text, solid_paint->color,
                                                            element_origin, framebuffer);
            }
            // TODO: Rasterize text with LinearGradientPaint instead of skipping and reporting an empty size.
            // resolved_paint keeps that gradient's start, end, and colors with the defining ancestor origin.
            return Size{};
        }

        Size operator()(const FilledRectangleElementPayload& payload) const
        {
            if (const SolidPaint* solid_paint = ElementTreeRenderer::as_solid_paint(resolved_paint)) {
                renderer.rectangle_rasterizer_.rasterize(payload.size, solid_paint->color, element_origin,
                                                          framebuffer);
                return payload.size;
            }
            // TODO: Rasterize filled rectangles with LinearGradientPaint instead of skipping the fill.
            // Layout size is retained. resolved_paint keeps the gradient and its defining ancestor origin.
            return payload.size;
        }
    };

    return std::visit(ElementPayloadVisitor{
                          .renderer = *this,
                          .tree = tree,
                          .node = node,
                          .element_origin = canvas_origin,
                          .resolved_paint = resolved_paint,
                          .framebuffer = framebuffer,
                      },
                      node.element.payload);
}

void ElementTreeRenderer::render(const ElementTree& tree, LogicalFramebuffer& framebuffer) const
{
    constexpr Position kCanvasOrigin{.x = 0, .y = 0};
    const Position root_origin = element_origin_on_canvas(kCanvasOrigin, tree.nodes[tree.root].element.position);
    static_cast<void>(render_node(tree, tree.root, root_origin, std::nullopt, framebuffer));
}

}  // namespace display
