#include "color_sampler.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <utility>
#include <variant>

namespace graphics {
namespace {

constexpr double kFullTurnDegrees = 360.0;
constexpr double kRightAngleDegrees = 90.0;
constexpr double kStraightAngleDegrees = 180.0;
constexpr double kThreeQuarterTurnDegrees = 270.0;

struct ScreenDirection {
    double x{0};
    double y{0};
};

// CSS angles start at up and turn clockwise. Cardinal angles are exact screen
// axes: sin/cos of a multiple of 90 degrees sits a fraction of pi off the axis
// and would tint an otherwise horizontal or vertical gradient.
[[nodiscard]] ScreenDirection screen_direction(float angle_degrees)
{
    double normalized = std::fmod(static_cast<double>(angle_degrees), kFullTurnDegrees);
    if (normalized < 0.0) {
        normalized += kFullTurnDegrees;
    }
    if (normalized == 0.0) {
        return {.x = 0.0, .y = -1.0};
    }
    if (normalized == kRightAngleDegrees) {
        return {.x = 1.0, .y = 0.0};
    }
    if (normalized == kStraightAngleDegrees) {
        return {.x = 0.0, .y = 1.0};
    }
    if (normalized == kThreeQuarterTurnDegrees) {
        return {.x = -1.0, .y = 0.0};
    }

    const double radians = normalized * (std::numbers::pi / kStraightAngleDegrees);
    return {.x = std::sin(radians), .y = -std::cos(radians)};
}

struct GradientAxis {
    double direction_x{0};
    double direction_y{0};
    double minimum_projection{0};
    double projection_span{0};
};

[[nodiscard]] GradientAxis gradient_axis(float angle_degrees, Position origin, Size size)
{
    const ScreenDirection direction = screen_direction(angle_degrees);
    const double left = static_cast<double>(origin.x);
    const double top = static_cast<double>(origin.y);
    const double right = left + static_cast<double>(size.width);
    const double bottom = top + static_cast<double>(size.height);

    const double top_left = left * direction.x + top * direction.y;
    const double top_right = right * direction.x + top * direction.y;
    const double bottom_left = left * direction.x + bottom * direction.y;
    const double bottom_right = right * direction.x + bottom * direction.y;
    const double minimum = std::min(std::min(top_left, top_right), std::min(bottom_left, bottom_right));
    const double maximum = std::max(std::max(top_left, top_right), std::max(bottom_left, bottom_right));
    return GradientAxis{
        .direction_x = direction.x,
        .direction_y = direction.y,
        .minimum_projection = minimum,
        .projection_span = maximum - minimum,
    };
}

[[nodiscard]] uint8_t interpolate_channel(uint8_t start, uint8_t end, double amount)
{
    const double blended =
        static_cast<double>(start) + (static_cast<double>(end) - static_cast<double>(start)) * amount;
    const double rounded = std::round(std::clamp(blended, 0.0, 255.0));
    return static_cast<uint8_t>(rounded);
}

[[nodiscard]] RgbColor interpolate_color(RgbColor start, RgbColor end, double amount)
{
    return RgbColor{
        .red = interpolate_channel(start.red, end.red, amount),
        .green = interpolate_channel(start.green, end.green, amount),
        .blue = interpolate_channel(start.blue, end.blue, amount),
    };
}

[[nodiscard]] uint8_t effective_stop_count(const LinearGradientPaint& gradient)
{
    constexpr auto kStopLimit = static_cast<uint8_t>(kMaxGradientColorStops);
    return gradient.color_stop_count < kStopLimit ? gradient.color_stop_count : kStopLimit;
}

[[nodiscard]] RgbColor sample_stops(const LinearGradientPaint& gradient, double offset)
{
    const uint8_t count = effective_stop_count(gradient);
    if (count == 0) {
        return {};
    }

    const GradientColorStop& first = gradient.color_stops[0];
    if (count == 1 || offset <= static_cast<double>(first.offset)) {
        return first.color;
    }

    const uint8_t last_index = static_cast<uint8_t>(count - 1);
    const GradientColorStop& last = gradient.color_stops[last_index];
    if (offset >= static_cast<double>(last.offset)) {
        return last.color;
    }

    for (uint8_t index = 0; index < last_index; ++index) {
        const GradientColorStop& left = gradient.color_stops[index];
        const GradientColorStop& right = gradient.color_stops[static_cast<uint8_t>(index + 1)];
        if (offset > static_cast<double>(right.offset)) {
            continue;
        }

        const double span = static_cast<double>(right.offset) - static_cast<double>(left.offset);
        if (span <= 0.0) {
            return right.color;
        }
        const double amount = (offset - static_cast<double>(left.offset)) / span;
        return interpolate_color(left.color, right.color, amount);
    }

    return last.color;
}

}  // namespace

ColorSampler::ColorSampler(Paint paint, Position paint_origin, Size paint_size)
    : paint_{std::move(paint)}
{
    const auto* gradient = std::get_if<LinearGradientPaint>(&paint_);
    if (gradient == nullptr) {
        return;
    }

    const GradientAxis axis = gradient_axis(gradient->angle_degrees, paint_origin, paint_size);
    direction_x_ = axis.direction_x;
    direction_y_ = axis.direction_y;
    minimum_projection_ = axis.minimum_projection;
    projection_span_ = axis.projection_span;
}

RgbColor ColorSampler::sample(Position canvas_position) const
{
    const auto* solid = std::get_if<SolidPaint>(&paint_);
    if (solid != nullptr) {
        return solid->color;
    }

    const auto* gradient = std::get_if<LinearGradientPaint>(&paint_);
    if (gradient == nullptr) {
        return {};
    }

    const double projected = static_cast<double>(canvas_position.x) * direction_x_
        + static_cast<double>(canvas_position.y) * direction_y_;
    const double offset =
        projection_span_ > 0.0 ? (projected - minimum_projection_) / projection_span_ : 0.0;
    return sample_stops(*gradient, offset);
}

}  // namespace graphics
