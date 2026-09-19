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
// non-container passed to `add_container`, or exhausted node storage) are
// sticky: later `add` / `add_container` calls return false without appending.
class ElementTreeBuilder {
public:
    explicit ElementTreeBuilder(std::span<ElementTreeNode> storage);

    ElementTreeBuilder(const ElementTreeBuilder&) = delete;
    ElementTreeBuilder& operator=(const ElementTreeBuilder&) = delete;

    [[nodiscard]] bool add(Element element);

    template <typename AddChildren>
    [[nodiscard]] bool add_container(Element container, AddChildren&& add_children);

    // Precondition: the build produced exactly one root and did not fail.
    [[nodiscard]] ElementTree get_tree() const;

private:
    struct SharedBuildState {
        std::span<ElementTreeNode> storage{};
        std::size_t used{0};
        bool failed{false};
        std::optional<ElementNodeIndex> root{};
    };

    explicit ElementTreeBuilder(SharedBuildState* state, ElementNodeIndex parent);

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

inline bool ElementTreeBuilder::add(Element element)
{
    if (state_->failed) {
        return false;
    }
    if (!parent_.has_value() && state_->root.has_value()) {
        state_->failed = true;
        return false;
    }

    constexpr std::size_t kMaxNodeCount =
        static_cast<std::size_t>(std::numeric_limits<ElementNodeIndex>::max()) + 1;
    if (state_->used >= state_->storage.size() || state_->used >= kMaxNodeCount) {
        state_->failed = true;
        return false;
    }

    const auto index = static_cast<ElementNodeIndex>(state_->used);
    ElementTreeNode& node = state_->storage[index];
    node.element = element;
    node.first_child.reset();
    node.next_sibling.reset();
    ++state_->used;

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
    return true;
}

template <typename AddChildren>
bool ElementTreeBuilder::add_container(Element container, AddChildren&& add_children)
{
    if (state_->failed) {
        return false;
    }
    if (std::get_if<ContainerElement>(&container.payload) == nullptr) {
        state_->failed = true;
        return false;
    }
    if (!add(container)) {
        return false;
    }

    const auto parent_index = static_cast<ElementNodeIndex>(state_->used - 1);
    ElementTreeBuilder child_scope{state_, parent_index};
    std::forward<AddChildren>(add_children)(child_scope);
    return !state_->failed;
}

inline ElementTree ElementTreeBuilder::get_tree() const
{
    assert(!state_->failed && state_->root.has_value());
    return ElementTree{
        .root = *state_->root,
        .nodes = state_->storage.first(state_->used),
    };
}

}  // namespace display
