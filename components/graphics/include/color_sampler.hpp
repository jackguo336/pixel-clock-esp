#pragma once

#include "color.hpp"
#include "geometry.hpp"

namespace graphics {

// Samples a paint across the defining element's pixel bounds. Solid paint is
// constant. A linear gradient runs along a defined angle and stops that cover the 
// whole rectangle.
class ColorSampler final {
public:
    ColorSampler(Paint paint, Position paint_origin, Size paint_size);

    [[nodiscard]] RgbColor sample(Position canvas_position) const;

private:
    Paint paint_{};
    double direction_x_{0};
    double direction_y_{0};
    double minimum_projection_{0};
    double projection_span_{0};
};

}  // namespace graphics
