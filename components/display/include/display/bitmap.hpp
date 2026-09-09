#pragma once

#include <span>

#include "display/elements.hpp"

namespace display {

struct BitmapView {
    Size size{};
    std::span<const RgbColor> pixels{};

    [[nodiscard]] bool is_valid() const;
};

struct MutableBitmapView {
    Size size{};
    std::span<RgbColor> pixels{};

    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] BitmapView as_read_only() const;
};

}  // namespace display
