#pragma once

#include <cstdint>

namespace display {

// Immutable, value-like snapshot of inputs for view building. Bounded and
// strongly typed: no service references or untyped property bags.
struct ViewData {
    uint32_t revision{0};
};

}  // namespace display
