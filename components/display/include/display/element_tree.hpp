#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <variant>

#include "display/elements.hpp"

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
// `add_container` callbacks share that buffer and link children in declaration
// order through indices, so growing the tree cannot invalidate node pointers.
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
    struct SharedBuildState {
        std::span<ElementTreeNode> storage{};
        std::size_t next_free_slot_index{0};
        bool failed_to_add_element{false};
        std::optional<ElementNodeIndex> root{};
    };

    explicit ElementTreeBuilder(SharedBuildState* state, ElementNodeIndex parent);

    [[nodiscard]] static bool is_terminal_element(const Element& element);
    [[nodiscard]] bool add(Element element);
    [[nodiscard]] std::optional<ElementNodeIndex> create_node(Element element);
    void link_node_to_tree(ElementNodeIndex index);

    SharedBuildState owned_state_{};
    SharedBuildState* state_{nullptr};
    std::optional<ElementNodeIndex> parent_{};
    std::optional<ElementNodeIndex> last_sibling_{};
};

inline ElementTreeBuilder::ElementTreeBuilder(std::span<ElementTreeNode> storage)
    : owned_state_{.storage = storage},
      state_(&owned_state_)
{
}

inline ElementTreeBuilder::ElementTreeBuilder(SharedBuildState* state, ElementNodeIndex parent)
    : state_(state),
      parent_(parent)
{
}

inline std::optional<ElementNodeIndex> ElementTreeBuilder::create_node(Element element)
{
    constexpr std::size_t kMaxNodeCount =
        static_cast<std::size_t>(std::numeric_limits<ElementNodeIndex>::max()) + 1;
    if (state_->next_free_slot_index >= state_->storage.size() ||
        state_->next_free_slot_index >= kMaxNodeCount) {
        state_->failed_to_add_element = true;
        return std::nullopt;
    }

    const auto index = static_cast<ElementNodeIndex>(state_->next_free_slot_index);
    ElementTreeNode& node = state_->storage[index];
    node.element = element;
    // Start with no children or sibling links for the newly created node.
    node.first_child.reset();
    node.next_sibling.reset();
    ++state_->next_free_slot_index;
    return index;
}

inline void ElementTreeBuilder::link_node_to_tree(ElementNodeIndex index)
{
    // 1. If no parent, tree is empty, so set node as root.
    // 2. If parent exists and has no children, set node as first child.
    // 3. If parent exists and has children, set node as next sibling of last child.
    if (parent_.has_value()) {
        ElementTreeNode& parent_node = state_->storage[*parent_];
        if (!parent_node.first_child.has_value()) {
            parent_node.first_child = index;
        } else if (last_sibling_.has_value()) {
            state_->storage[*last_sibling_].next_sibling = index;
        }
        last_sibling_ = index;
    } else {
        state_->root = index;
    }
}

inline bool ElementTreeBuilder::is_terminal_element(const Element& element)
{
    return std::get_if<TextElement>(&element.payload) != nullptr
        || std::get_if<FilledRectangleElement>(&element.payload) != nullptr
        || std::get_if<BitmapElement>(&element.payload) != nullptr;
}

inline bool ElementTreeBuilder::add(Element element)
{
    if (state_->failed_to_add_element) {
        return false;
    }
    if (!parent_.has_value() && state_->root.has_value()) {
        state_->failed_to_add_element = true;
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
    if (state_->failed_to_add_element) {
        return false;
    }
    if (!is_terminal_element(element)) {
        state_->failed_to_add_element = true;
        return false;
    }
    return add(element);
}

template <typename AddChildren>
bool ElementTreeBuilder::add_container(Element container, AddChildren&& add_children)
{
    if (state_->failed_to_add_element) {
        return false;
    }
    if (std::get_if<ContainerElement>(&container.payload) == nullptr) {
        state_->failed_to_add_element = true;
        return false;
    }
    if (!add(container)) {
        return false;
    }

    const auto parent_index = static_cast<ElementNodeIndex>(state_->next_free_slot_index - 1);
    ElementTreeBuilder child_scope{state_, parent_index};
    std::forward<AddChildren>(add_children)(child_scope);
    return !state_->failed_to_add_element;
}

inline ElementTree ElementTreeBuilder::get_tree() const
{
    assert(!state_->failed_to_add_element && state_->root.has_value());
    return ElementTree{
        .root = *state_->root,
        .nodes = state_->storage.first(state_->next_free_slot_index),
    };
}

}  // namespace display
