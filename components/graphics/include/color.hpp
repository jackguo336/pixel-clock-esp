#pragma once

#include <cstdint>
#include <variant>

#include "geometry.hpp"

namespace display {

struct RgbColor {
    uint8_t red{0};
    uint8_t green{0};
    uint8_t blue{0};
};

struct SolidPaint {
    RgbColor color{};
};

struct LinearGradientPaint {
    Position start{};
    Position end{};
    RgbColor start_color{};
    RgbColor end_color{};
};

using Paint = std::variant<SolidPaint, LinearGradientPaint>;

}  // namespace display
