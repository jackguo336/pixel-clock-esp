#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

#include "bitmap_file.hpp"
#include "color.hpp"
#include "font.hpp"
#include "geometry.hpp"

namespace display {

struct ElementId {
    uint16_t value{0};
};

struct TextElementPayload {
    std::string_view text{};
    // Reference to a font in memory.
    const Font* font{};
};

struct FilledRectangleElementPayload {
    Size size{};
};

struct BitmapElementPayload {
    // Reference to a bitmap file in memory.
    const BitmapFile* bitmap{};
};

enum class StackDirection : uint8_t {
    TopToBottom = 0,
    LeftToRight = 1,
};

enum class LayoutSystem : uint8_t {
    ChildDefinedPositions = 0,
    Stacked = 1,
};

struct ContainerElementPayload {
    StackDirection layout_direction{};
    LayoutSystem layout_system{};
    // Container children are defined in the ElementTree.
};

using ElementPayload =
    std::variant<TextElementPayload, FilledRectangleElementPayload, BitmapElementPayload,
                 ContainerElementPayload>;

struct Element {
    ElementId id{};
    Position position{};
    // Unset paint inherits the nearest ancestor's paint during rendering.
    std::optional<Paint> paint{};
    ElementPayload payload{};
};

}  // namespace display
