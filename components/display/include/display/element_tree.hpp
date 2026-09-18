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
#include "display/view_data.hpp"

namespace display {

class View;

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
// non-container passed to `add_container`, exhausted node storage, or
// `add_view` receiving a false result / zero / multiple direct subtrees) are
// sticky: later `add` / `add_container` / `add_view` calls return false
// without appending.
class ElementTreeBuilder {
public:
    explicit ElementTreeBuilder(std::span<ElementTreeNode> storage);

    ElementTreeBuilder(const ElementTreeBuilder&) = delete;
    ElementTreeBuilder& operator=(const ElementTreeBuilder&) = delete;

    [[nodiscard]] bool add(Element element);

    template <typename AddChildren>
    [[nodiscard]] bool add_container(Element container, AddChildren&& add_children);

    // Nested-view composition: synchronously builds `view` into this scope.
    // The view must contribute exactly one direct subtree. Generated elements
    // are stored; the View object is not.
    [[nodiscard]] bool add_view(const View& view, const ViewData& view_data);

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

    [[nodiscard]] std::size_t count_direct_subtrees_added_since(
        std::optional<ElementNodeIndex> last_sibling_before,
        bool had_root_before) const;

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

inline std::size_t ElementTreeBuilder::count_direct_subtrees_added_since(
    std::optional<ElementNodeIndex> last_sibling_before,
    bool had_root_before) const
{
    if (!parent_.has_value()) {
        if (!state_->root.has_value() || had_root_before) {
            return 0;
        }
        return 1;
    }

    std::optional<ElementNodeIndex> current{};
    if (last_sibling_before.has_value()) {
        current = state_->storage[*last_sibling_before].next_sibling;
    } else {
        current = state_->storage[*parent_].first_child;
    }

    std::size_t count = 0;
    while (current.has_value()) {
        ++count;
        current = state_->storage[*current].next_sibling;
    }
    return count;
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
