#include "element_tree_renderer.hpp"

#include <optional>
#include <variant>

#include "display/elements.hpp"
#include "display/geometry.hpp"

namespace display {
namespace {

[[nodiscard]] Position element_origin_on_canvas(Position parent_origin, Position element_position)
{
    return Position{
        .x = static_cast<int16_t>(parent_origin.x + element_position.x),
        .y = static_cast<int16_t>(parent_origin.y + element_position.y),
    };
}

void render_node(const ElementTree& tree, ElementNodeIndex index, Position parent_origin,
                 LogicalFramebuffer& framebuffer, const BitmapRasterizer& bitmap_rasterizer);

void render_children(const ElementTree& tree, std::optional<ElementNodeIndex> first_child,
                     Position container_origin, LogicalFramebuffer& framebuffer,
                     const BitmapRasterizer& bitmap_rasterizer)
{
    std::optional<ElementNodeIndex> child_index = first_child;
    while (child_index.has_value()) {
        render_node(tree, *child_index, container_origin, framebuffer, bitmap_rasterizer);
        child_index = tree.nodes[*child_index].next_sibling;
    }
}

struct ElementPayloadVisitor {
    const ElementTree& tree;
    const ElementTreeNode& node;
    Position element_origin;
    LogicalFramebuffer& framebuffer;
    const BitmapRasterizer& bitmap_rasterizer;

    void operator()(const ContainerElementPayload&) const
    {
        render_children(tree, node.first_child, element_origin, framebuffer, bitmap_rasterizer);
    }

    void operator()(const BitmapElementPayload& payload) const
    {
        if (payload.bitmap != nullptr) {
            bitmap_rasterizer.rasterize(*payload.bitmap, element_origin, framebuffer);
        }
    }

    void operator()(const TextElementPayload&) const {}

    void operator()(const FilledRectangleElementPayload&) const {}
};

void render_node(const ElementTree& tree, ElementNodeIndex index, Position parent_origin,
                 LogicalFramebuffer& framebuffer, const BitmapRasterizer& bitmap_rasterizer)
{
    const ElementTreeNode& node = tree.nodes[index];
    const Position element_origin = element_origin_on_canvas(parent_origin, node.element.position);
    std::visit(ElementPayloadVisitor{
                   .tree = tree,
                   .node = node,
                   .element_origin = element_origin,
                   .framebuffer = framebuffer,
                   .bitmap_rasterizer = bitmap_rasterizer,
               },
               node.element.payload);
}

}  // namespace

void ElementTreeRenderer::render(const ElementTree& tree, LogicalFramebuffer& framebuffer) const
{
    constexpr Position kCanvasOrigin{.x = 0, .y = 0};
    render_node(tree, tree.root, kCanvasOrigin, framebuffer, bitmap_rasterizer_);
}

}  // namespace display
