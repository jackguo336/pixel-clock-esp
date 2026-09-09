#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "display/elements.hpp"

namespace display {

class LogicalFramebuffer final {
public:
    static constexpr int32_t kWidth = 32;
    static constexpr int32_t kHeight = 8;
    static constexpr size_t kPixelCount = kWidth * kHeight;

    void clear(RgbColor color = {});
    [[nodiscard]] bool set_pixel(int32_t x, int32_t y, RgbColor color);
    [[nodiscard]] const RgbColor* pixel_at(int32_t x, int32_t y) const;
    [[nodiscard]] std::span<const RgbColor, kPixelCount> pixels() const;

private:
    [[nodiscard]] bool contains(int32_t x, int32_t y) const;
    [[nodiscard]] size_t index_unchecked(int32_t x, int32_t y) const;

    std::array<RgbColor, kPixelCount> pixels_{};
};

}  // namespace display
