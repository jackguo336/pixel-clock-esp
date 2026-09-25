#include "led_matrix_map.hpp"

#include "display/logical_framebuffer.hpp"

namespace led_matrix {
namespace {

struct LanePlacement {
    int32_t lane{0};
    int32_t along{0};
    int32_t lane_length{0};
};

/**
 * Map a logical pixel onto the physical LED strip.
 *
 * A lane is one straight run of the strip. Horizontal wiring uses one lane per row;
 * vertical wiring uses one lane per column. `along` is the offset within that run.
 * The strip index is `lane * lane_length + along`. Serpentine wiring reverses every
 * odd lane. Reversed orientations start lane 0 from the opposite end.
 *
 * Horizontal serpentine (numbers are strip indices):
 *
 *        x=0   x=1   x=2   x=3
 *   y=0   0  ->  1  ->  2  ->  3     lane 0, along = x
 *   y=1   7  <-  6  <-  5  <-  4     lane 1, along = width - 1 - x
 *   y=2   8  ->  9  -> 10  -> 11     lane 2, along = x
 *
 * Vertical serpentine:
 *
 *          lane 0    lane 1    lane 2
 *             |         |         |
 *             v         ^         v
 *   y=0       0         5         6
 *   y=1       1         4         7
 *   y=2       2         3         8
 */
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
