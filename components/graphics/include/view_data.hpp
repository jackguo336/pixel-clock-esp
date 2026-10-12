#pragma once

#include <chrono>
#include <cstdint>

namespace graphics {

// Immutable, value-like snapshot of inputs for view building. Bounded and
// strongly typed: no service references or untyped property bags.
struct ViewData {
    uint32_t revision{0};
    // Monotonic time since boot, sampled on every display refresh.
    std::chrono::microseconds timestamp{0};
};

}  // namespace graphics
