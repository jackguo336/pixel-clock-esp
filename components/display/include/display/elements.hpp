#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "display/bitmap.hpp"
#include "display/color.hpp"
#include "display/geometry.hpp"

namespace display {

struct ElementId {
    uint16_t value{0};
};

struct TextElementPayload {
    std::string_view text{};
};

struct FilledRectangleElementPayload {
    Size size{};
};

struct BitmapElementPayload {
    // Reference to a static bitmap file in memory.
    const BitmapFile* bitmap{};
};

enum class LayoutDirection : uint8_t {
    TopToBottom = 0,
    LeftToRight = 1,
    RightToLeft = 2,
    BottomToTop = 3,
};

struct ContainerElementPayload {
    LayoutDirection layout_direction{};
    // Container children are defined in the ElementTree.
};

using ElementPayload =
    std::variant<TextElementPayload, FilledRectangleElementPayload, BitmapElementPayload,
                 ContainerElementPayload>;

struct Element {
    ElementId id{};
    Position position{};
    Paint paint{};
    ElementPayload payload{};
};

}  // namespace display
