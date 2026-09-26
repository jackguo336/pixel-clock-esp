#include "logical_framebuffer.hpp"

namespace display {

void LogicalFramebuffer::clear(RgbColor color)
{
    pixels_.fill(color);
}

bool LogicalFramebuffer::set_pixel(int32_t x, int32_t y, RgbColor color)
{
    if (!contains(x, y)) {
        return false;
    }

    pixels_[index_unchecked(x, y)] = color;
    return true;
}

const RgbColor* LogicalFramebuffer::pixel_at(int32_t x, int32_t y) const
{
    if (!contains(x, y)) {
        return nullptr;
    }

    return &pixels_[index_unchecked(x, y)];
}

std::span<const RgbColor, LogicalFramebuffer::kPixelCount> LogicalFramebuffer::pixels() const
{
    return pixels_;
}

bool LogicalFramebuffer::contains(int32_t x, int32_t y) const
{
    return x >= 0 && y >= 0 && x < kWidth && y < kHeight;
}

size_t LogicalFramebuffer::index_unchecked(int32_t x, int32_t y) const
{
    return static_cast<size_t>(y) * static_cast<size_t>(kWidth) + static_cast<size_t>(x);
}

}  // namespace display
