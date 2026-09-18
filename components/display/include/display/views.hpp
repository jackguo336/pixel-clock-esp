#pragma once

#include "display/element_tree.hpp"
#include "display/view_data.hpp"

namespace display {

// Builds one bounded element tree from an immutable ViewData snapshot.
//
// Views do not own the builder's output nodes. Any text or bitmap backing
// data referenced by those nodes must remain alive for as long as the
// resulting tree is used. Builder failures are sticky; build() must
// propagate them by returning false. Nested views are composed with
// ElementTreeBuilder::add_view, which stores generated elements only.
class View {
public:
    virtual ~View() = default;

    [[nodiscard]] virtual bool build(
        const ViewData& view_data,
        ElementTreeBuilder& builder) const = 0;
};

inline bool ElementTreeBuilder::add_view(const View& view, const ViewData& view_data)
{
    if (state_->failed) {
        return false;
    }

    const auto last_sibling_before = last_sibling_;
    const bool had_root_before = state_->root.has_value();
    if (!view.build(view_data, *this)) {
        state_->failed = true;
        return false;
    }
    if (state_->failed) {
        return false;
    }
    if (count_direct_subtrees_added_since(last_sibling_before, had_root_before) != 1) {
        state_->failed = true;
        return false;
    }
    return true;
}

}  // namespace display
