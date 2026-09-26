#pragma once

#include "element_tree.hpp"
#include "view_data.hpp"

namespace display {

// Builds one bounded element tree from an immutable ViewData snapshot.
//
// Views do not own the builder's output nodes. Any text or bitmap backing
// data referenced by those nodes must remain alive for as long as the
// resulting tree is used. Builder failures are sticky; build() must
// propagate them by returning false.
class View {
public:
    virtual ~View() = default;

    [[nodiscard]] virtual bool build(
        const ViewData& view_data,
        ElementTreeBuilder& builder) const = 0;
};

}  // namespace display
