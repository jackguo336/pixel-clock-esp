#include "rectangle_rasterizer.hpp"

#include <algorithm>
#include <cstdint>

namespace graphics {

void RectangleRasterizer::rasterize(Size size, RgbColor color, Position canvas_origin,
                                     LogicalFramebuffer& framebuffer) const
{
    if (size.width == 0 || size.height == 0) {
        return;
    }

    // Only draw pixels inside the framebuffer boundary that will be visible.
    const int32_t rectangle_left = canvas_origin.x;
    const int32_t rectangle_top = canvas_origin.y;
    const int32_t rectangle_right = rectangle_left + static_cast<int32_t>(size.width);
    const int32_t rectangle_bottom = rectangle_top + static_cast<int32_t>(size.height);

    const int32_t visible_left = std::max(rectangle_left, int32_t{0});
    const int32_t visible_top = std::max(rectangle_top, int32_t{0});
    const int32_t visible_right = std::min(rectangle_right, LogicalFramebuffer::kWidth);
    const int32_t visible_bottom = std::min(rectangle_bottom, LogicalFramebuffer::kHeight);

    for (int32_t y = visible_top; y < visible_bottom; ++y) {
        for (int32_t x = visible_left; x < visible_right; ++x) {
            static_cast<void>(framebuffer.set_pixel(x, y, color));
        }
    }
}

}  // namespace graphics
