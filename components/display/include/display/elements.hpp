#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

#include "display/common.hpp"

namespace display {

struct WidgetId {
    uint16_t value{0};
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

struct TextElement {
    FontId font_id{};
    // Immutable text stored elsewhere (typically a static scene definition).
    std::string_view text{};
};

struct FilledRectangleElement {
    Size size{};
};

struct BitmapElement {
    BitmapId bitmap_id{};
};

enum class LayoutDirection : uint8_t {
    TopToBottom = 0,
    LeftToRight = 1,
    RightToLeft = 2,
    BottomToTop = 3,
};

using ContainerChild = std::variant<ElementId, WidgetId>;

struct ContainerElement {
    LayoutDirection layout_direction{};
    // Child IDs in a future scene's static storage. The container's position is
    // the origin of that local coordinate space; children are not owned here.
    std::span<const ContainerChild> children{};
};

// Closed set of element payloads. Inspect with std::get_if (no RTTI, no throw).
using ElementPayload =
    std::variant<TextElement, FilledRectangleElement, BitmapElement, ContainerElement>;

struct Element {
    ElementId id{};
    Position position{};
    Paint paint{};
    ElementPayload payload{};
};

}  // namespace display
