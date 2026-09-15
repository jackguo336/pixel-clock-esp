#pragma once

#include <cstdint>

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

}  // namespace display
