#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

namespace display {

struct ElementId {
    uint16_t value{0};
};

struct FontId {
    uint16_t value{0};
};

struct BitmapId {
    uint16_t value{0};
};

struct Position {
    int16_t x{0};
    int16_t y{0};
};

struct Size {
    uint16_t width{0};
    uint16_t height{0};
};

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

struct ContainerElement {
    // Child IDs in a future scene's static storage. The container's position is
    // the origin of that local coordinate space; children are not owned here.
    std::span<const ElementId> child_ids{};
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
