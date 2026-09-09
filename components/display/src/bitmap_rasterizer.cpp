#include "bitmap_rasterizer.hpp"

#include <cstddef>
#include <cstdint>

namespace display {

void BitmapRasterizer::rasterize(BitmapView bitmap, Position canvas_origin,
                                 LogicalFramebuffer& framebuffer) const
{
    if (!bitmap.is_valid()) {
        return;
    }

    const int32_t origin_x = canvas_origin.x;
    const int32_t origin_y = canvas_origin.y;
    const int32_t width = static_cast<int32_t>(bitmap.size.width);
    const int32_t height = static_cast<int32_t>(bitmap.size.height);

    for (int32_t row = 0; row < height; ++row) {
        const int32_t canvas_y = origin_y + row;
        const std::size_t row_offset =
            static_cast<std::size_t>(row) * static_cast<std::size_t>(width);
        for (int32_t column = 0; column < width; ++column) {
            const int32_t canvas_x = origin_x + column;
            const RgbColor color = bitmap.pixels[row_offset + static_cast<std::size_t>(column)];
            static_cast<void>(framebuffer.set_pixel(canvas_x, canvas_y, color));
        }
    }
}

}  // namespace display
