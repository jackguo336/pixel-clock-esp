#pragma once

#include <cstdint>

namespace display {

// Immutable, value-like snapshot of inputs for widget resolution. Bounded and
// strongly typed: no service references or untyped property bags.
struct SceneData {
    uint32_t revision{0};
};

}  // namespace display
