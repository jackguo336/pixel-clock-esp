#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <variant>

#include "elements.hpp"

namespace display {

using ElementNodeIndex = uint16_t;

struct ElementTreeNode {
    Element element{};
    std::optional<ElementNodeIndex> first_child{};
    std::optional<ElementNodeIndex> next_sibling{};
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

    // Precondition: the build produced exactly one root and did not fail.
    [[nodiscard]] ElementTree get_tree() const;

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

inline ElementTreeBuilder::ElementTreeBuilder(std::span<ElementTreeNode> storage)
    : storage_(storage)
{
}

inline std::optional<ElementNodeIndex> ElementTreeBuilder::create_node(Element element)
{
    constexpr std::size_t kMaxNodeCount =
        static_cast<std::size_t>(std::numeric_limits<ElementNodeIndex>::max()) + 1;
    if (next_free_slot_index_ >= storage_.size() || next_free_slot_index_ >= kMaxNodeCount) {
        failed_to_add_element_ = true;
        return std::nullopt;
    }

    const auto index = static_cast<ElementNodeIndex>(next_free_slot_index_);
    ElementTreeNode& node = storage_[index];
    node.element = element;
    // Start with no children or sibling links for the newly created node.
    node.first_child.reset();
    node.next_sibling.reset();
    ++next_free_slot_index_;
    return index;
}

inline void ElementTreeBuilder::link_node_to_tree(ElementNodeIndex index)
{
    // 1. If no parent, tree is empty, so set node as root.
    // 2. If parent exists and has no children, set node as first child.
    // 3. If parent exists and has children, set node as next sibling of last child.
    if (parent_.has_value()) {
        ElementTreeNode& parent_node = storage_[*parent_];
        if (!parent_node.first_child.has_value()) {
            parent_node.first_child = index;
        } else if (last_sibling_.has_value()) {
            storage_[*last_sibling_].next_sibling = index;
        }
        last_sibling_ = index;
    } else {
        root_ = index;
    }
}

inline bool ElementTreeBuilder::is_terminal_element(const Element& element)
{
    return std::get_if<TextElementPayload>(&element.payload) != nullptr
        || std::get_if<FilledRectangleElementPayload>(&element.payload) != nullptr
        || std::get_if<BitmapElementPayload>(&element.payload) != nullptr;
}

inline bool ElementTreeBuilder::add(Element element)
{
    if (failed_to_add_element_) {
        return false;
    }
    if (!parent_.has_value() && root_.has_value()) {
        failed_to_add_element_ = true;
        return false;
    }

    const std::optional<ElementNodeIndex> created_index = create_node(element);
    if (!created_index.has_value()) {
        return false;
    }
    link_node_to_tree(*created_index);
    return true;
}

inline bool ElementTreeBuilder::add_terminal(Element element)
{
    if (failed_to_add_element_) {
        return false;
    }
    if (!is_terminal_element(element)) {
        failed_to_add_element_ = true;
        return false;
    }
    return add(element);
}

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

inline ElementTree ElementTreeBuilder::get_tree() const
{
    assert(!failed_to_add_element_ && root_.has_value());
    return ElementTree{
        .root = *root_,
        .nodes = storage_.first(next_free_slot_index_),
    };
}

}  // namespace display
