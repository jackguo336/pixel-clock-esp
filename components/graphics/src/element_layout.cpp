#include "element_layout.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <variant>

#include "elements.hpp"
#include "text_measurement.hpp"

namespace graphics {
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

[[nodiscard]] Size layout_node(std::span<ElementTreeNode> nodes, ElementNodeIndex index, Position origin_on_canvas);

[[nodiscard]] Size layout_children(std::span<ElementTreeNode> nodes, std::optional<ElementNodeIndex> first_child,
                                   Position container_origin, const ContainerElementPayload& container)
{
    int32_t max_right = 0;
    int32_t max_bottom = 0;
    Position previous_position{};
    Size previous_size{};
    bool has_previous_sibling = false;

    std::optional<ElementNodeIndex> child_index = first_child;
    while (child_index.has_value()) {
        const ElementTreeNode& child = nodes[*child_index];
        const Position relative_position = child_relative_position(
            child.element, container, has_previous_sibling, previous_position, previous_size);
        const Position child_origin = element_origin_on_canvas(container_origin, relative_position);
        const Size child_size = layout_node(nodes, *child_index, child_origin);

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

Size layout_node(std::span<ElementTreeNode> nodes, ElementNodeIndex index, Position origin_on_canvas)
{
    ElementTreeNode& node = nodes[index];

    struct ElementLayoutVisitor {
        std::span<ElementTreeNode> nodes;
        const ElementTreeNode& node;
        Position origin_on_canvas;

        Size operator()(const ContainerElementPayload& payload) const
        {
            return layout_children(nodes, node.first_child, origin_on_canvas, payload);
        }

        Size operator()(const BitmapElementPayload& payload) const
        {
            if (payload.bitmap == nullptr) {
                return {};
            }
            return payload.bitmap->size;
        }

        Size operator()(const TextElementPayload& payload) const
        {
            if (payload.font == nullptr || !payload.font->is_valid()) {
                return {};
            }
            return measure_text(*payload.font, payload.text);
        }

        Size operator()(const FilledRectangleElementPayload& payload) const
        {
            return payload.size;
        }
    };

    const Size size = std::visit(ElementLayoutVisitor{
                                     .nodes = nodes,
                                     .node = node,
                                     .origin_on_canvas = origin_on_canvas,
                                 },
                                 node.element.payload);
    node.layout = ElementLayout{
        .origin_on_canvas = origin_on_canvas,
        .size = size,
    };
    return size;
}

}  // namespace

void lay_out_element_tree(std::span<ElementTreeNode> nodes, ElementNodeIndex root)
{
    if (nodes.empty()) {
        return;
    }

    constexpr Position kCanvasOrigin{};
    const Position root_origin = element_origin_on_canvas(kCanvasOrigin, nodes[root].element.position);
    static_cast<void>(layout_node(nodes, root, root_origin));
}

}  // namespace graphics
