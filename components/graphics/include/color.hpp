#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>

#include "geometry.hpp"

namespace graphics {

struct RgbColor {
    uint8_t red{0};
    uint8_t green{0};
    uint8_t blue{0};
};

struct SolidPaint {
    RgbColor color{};
};

inline constexpr std::size_t kMaxGradientColorStops = 10;

struct GradientColorStop {
    float offset{};  // normalized [0.0, 1.0]
    RgbColor color{};
};

struct LinearGradientPaint {
    float angle_degrees{};  // CSS convention: 0 degrees up, 90 degrees right
    uint8_t color_stop_count{};
    std::array<GradientColorStop, kMaxGradientColorStops> color_stops{};
};

using Paint = std::variant<SolidPaint, LinearGradientPaint>;

}  // namespace graphics
