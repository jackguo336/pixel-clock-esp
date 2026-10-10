#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>

#include "elements.hpp"
#include "geometry.hpp"

namespace graphics {

using ElementNodeIndex = uint16_t;

struct ElementLayout {
    // Top left corner position of the element on the canvas
    Position origin_on_canvas{};
    Size size{};
};

struct ElementTreeNode {
    Element element{};
    std::optional<ElementNodeIndex> first_child{};
    std::optional<ElementNodeIndex> next_sibling{};
    // Populated when the element tree is built.
    ElementLayout layout{};
};

// Non-owning view of a completed tree. Valid while the caller-provided node
// storage used to build it remains alive.
struct ElementTree {
    ElementNodeIndex root{};
    std::span<const ElementTreeNode> nodes{};
};

// Builds one bounded element tree into caller-owned node storage. Nested
// `add_container` callbacks rebind this builder's parent and last-sibling
// cursors so children are linked in declaration order through indices.
// Growing the tree cannot invalidate node pointers.
//
// The top-level builder accepts exactly one root. Failures (a second root, a
// non-container passed to `add_container`, a non-terminal passed to
// `add_terminal`, or exhausted node storage) are sticky: later `add_terminal` /
// `add_container` calls return false without appending.
class ElementTreeBuilder {
public:
    explicit ElementTreeBuilder(std::span<ElementTreeNode> storage);

    ElementTreeBuilder(const ElementTreeBuilder&) = delete;
    ElementTreeBuilder& operator=(const ElementTreeBuilder&) = delete;

    [[nodiscard]] bool add_terminal(Element element);

    template <typename AddChildren>
    [[nodiscard]] bool add_container(Element container, AddChildren&& add_children);

    // Lays out every used node, then returns the finished tree.
    [[nodiscard]] ElementTree build();

private:
    [[nodiscard]] static bool is_terminal_element(const Element& element);
    [[nodiscard]] bool add(Element element);
    [[nodiscard]] std::optional<ElementNodeIndex> create_node(Element element);
    void link_node_to_tree(ElementNodeIndex index);

    std::span<ElementTreeNode> storage_{};
    std::size_t next_free_slot_index_{0};
    bool failed_to_add_element_{false};
    std::optional<ElementNodeIndex> root_{};
    std::optional<ElementNodeIndex> parent_{};
    std::optional<ElementNodeIndex> last_sibling_{};
};

template <typename AddChildren>
bool ElementTreeBuilder::add_container(Element container, AddChildren&& add_children)
{
    if (failed_to_add_element_) {
        return false;
    }
    if (std::get_if<ContainerElementPayload>(&container.payload) == nullptr) {
        failed_to_add_element_ = true;
        return false;
    }
    if (!add(container)) {
        return false;
    }

    // Save the current parent and last sibling to restore after adding children.
    const auto saved_parent = parent_;
    const auto saved_last_sibling = last_sibling_;

    // Set the parent to the new container so children can be added to it.
    parent_ = static_cast<ElementNodeIndex>(next_free_slot_index_ - 1);
    last_sibling_.reset();
    std::forward<AddChildren>(add_children)(*this);

    // Restore the parent and last sibling to their original values.
    parent_ = saved_parent;
    last_sibling_ = saved_last_sibling;
    return !failed_to_add_element_;
}

}  // namespace graphics
