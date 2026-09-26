#pragma once

#include <span>

#include "color.hpp"
#include "geometry.hpp"

namespace display {

struct BitmapFile {
    Size size{};
    std::span<const RgbColor> pixels{};

    [[nodiscard]] bool is_valid() const;
};

struct MutableBitmapFile {
    Size size{};
    std::span<RgbColor> pixels{};

    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] BitmapFile as_read_only() const;
};

}  // namespace display
