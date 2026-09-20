#pragma once

#include <cstdint>

namespace display {

struct Position {
    int16_t x{0};
    int16_t y{0};
};

struct Size {
    uint16_t width{0};
    uint16_t height{0};
};

}  // namespace display
