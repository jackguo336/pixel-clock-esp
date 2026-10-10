#include "element_tree_renderer.hpp"

#include <optional>
#include <variant>

#include "bitmap_rasterizer.hpp"
#include "color_sampler.hpp"
#include "elements.hpp"
#include "rectangle_rasterizer.hpp"
#include "text_rasterizer.hpp"

namespace graphics {
namespace {

struct ResolvedPaint {
    // Paint to apply for a node
    Paint paint{};
    // Origin of the element that defined the paint
    Position origin{};
    // Computed size of the element that defined the paint
    Size size{};
};

struct RenderContext {
    const ElementTree& tree;
    LogicalFramebuffer& framebuffer;
    BitmapRasterizer bitmap_rasterizer{};
    TextRasterizer text_rasterizer{};
    RectangleRasterizer rectangle_rasterizer{};
    // Allows the ancestor to apply its paint to children without paint.
    // Child can still override an ancestor's paint by setting its own paint.
    std::optional<ResolvedPaint> inherited_paint{};
};

[[nodiscard]] std::optional<ResolvedPaint> resolve_paint(const std::optional<Paint>& element_paint,
                                                         Position element_origin, Size element_size,
                                                         const std::optional<ResolvedPaint>& inherited_paint)
{
    if (!element_paint.has_value()) {
        return inherited_paint;
    }
    return ResolvedPaint{
        .paint = *element_paint,
        .origin = element_origin,
        .size = element_size,
    };
}

[[nodiscard]] ColorSampler color_sampler_for(const ResolvedPaint& resolved_paint)
{
    return ColorSampler{resolved_paint.paint, resolved_paint.origin, resolved_paint.size};
}

void render_node(RenderContext& context, ElementNodeIndex index);

void render_children(RenderContext& context, std::optional<ElementNodeIndex> first_child)
{
    std::optional<ElementNodeIndex> child_index = first_child;
    while (child_index.has_value()) {
        render_node(context, *child_index);
        child_index = context.tree.nodes[*child_index].next_sibling;
    }
}

void render_node(RenderContext& context, ElementNodeIndex index)
{
    const ElementTreeNode& node = context.tree.nodes[index];
    const ElementLayout& layout = node.layout;
    const auto saved_paint = context.inherited_paint;
    context.inherited_paint =
        resolve_paint(node.element.paint, layout.origin_on_canvas, layout.size, context.inherited_paint);

    struct ElementRenderVisitor {
        RenderContext& context;
        const ElementTreeNode& node;
        const ElementLayout& layout;

        void operator()(const ContainerElementPayload&) const
        {
            render_children(context, node.first_child);
        }

        void operator()(const BitmapElementPayload& payload) const
        {
            if (payload.bitmap == nullptr) {
                return;
            }
            context.bitmap_rasterizer.rasterize(*payload.bitmap, layout.origin_on_canvas, context.framebuffer);
        }

        void operator()(const TextElementPayload& payload) const
        {
            if (payload.font == nullptr || !payload.font->is_valid()) {
                return;
            }
            if (!context.inherited_paint.has_value()) {
                return;
            }
            context.text_rasterizer.rasterize(*payload.font, payload.text,
                                               color_sampler_for(*context.inherited_paint), layout.origin_on_canvas,
                                               context.framebuffer);
        }

        void operator()(const FilledRectangleElementPayload& payload) const
        {
            if (!context.inherited_paint.has_value()) {
                return;
            }
            context.rectangle_rasterizer.rasterize(payload.size, color_sampler_for(*context.inherited_paint),
                                                    layout.origin_on_canvas, context.framebuffer);
        }
    };

    std::visit(ElementRenderVisitor{
                   .context = context,
                   .node = node,
                   .layout = layout,
               },
               node.element.payload);
    context.inherited_paint = saved_paint;
}

}  // namespace

void ElementTreeRenderer::render(const ElementTree& tree, LogicalFramebuffer& framebuffer) const
{
    if (tree.nodes.empty()) {
        return;
    }
    RenderContext context{
        .tree = tree,
        .framebuffer = framebuffer,
    };
    render_node(context, tree.root);
}

}  // namespace graphics
