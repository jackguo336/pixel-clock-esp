#include "led_matrix_map.hpp"

#include "display/logical_framebuffer.hpp"

namespace led_matrix {
namespace {

struct LanePlacement {
    int32_t lane{0};
    int32_t along{0};
    int32_t lane_length{0};
};

[[nodiscard]] LanePlacement place_on_lane(int32_t x, int32_t y, LedMatrixOrientation orientation,
                                          bool serpentine)
{
    const int32_t width = display::LogicalFramebuffer::kWidth;
    const int32_t height = display::LogicalFramebuffer::kHeight;
    const bool horizontal = orientation == LedMatrixOrientation::Horizontal ||
                            orientation == LedMatrixOrientation::HorizontalReversed;
    const int32_t lane = horizontal ? y : x;
    const bool reverse_lane = serpentine && (lane % 2) != 0;

    switch (orientation) {
    case LedMatrixOrientation::Horizontal:
        return LanePlacement{
            .lane = y,
            .along = reverse_lane ? (width - 1 - x) : x,
            .lane_length = width,
        };
    case LedMatrixOrientation::HorizontalReversed:
        return LanePlacement{
            .lane = y,
            .along = reverse_lane ? x : (width - 1 - x),
            .lane_length = width,
        };
    case LedMatrixOrientation::Vertical:
        return LanePlacement{
            .lane = x,
            .along = reverse_lane ? (height - 1 - y) : y,
            .lane_length = height,
        };
    case LedMatrixOrientation::VerticalReversed:
        return LanePlacement{
            .lane = x,
            .along = reverse_lane ? y : (height - 1 - y),
            .lane_length = height,
        };
    }
    return LanePlacement{};
}

}  // namespace

std::size_t led_index_for_logical_pixel(int32_t x, int32_t y, LedMatrixOrientation orientation,
                                        bool serpentine)
{
    const LanePlacement placement = place_on_lane(x, y, orientation, serpentine);
    return static_cast<std::size_t>(placement.lane) * static_cast<std::size_t>(placement.lane_length) +
           static_cast<std::size_t>(placement.along);
}

}  // namespace led_matrix
