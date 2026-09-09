#include "display/bitmap.hpp"

#include <cstddef>

namespace display {
namespace {

[[nodiscard]] bool has_matching_pixel_count(Size size, std::size_t pixel_count)
{
    if (size.width == 0 || size.height == 0) {
        return false;
    }

    const std::size_t expected =
        static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height);
    return pixel_count == expected;
}

}  // namespace

bool BitmapView::is_valid() const
{
    return has_matching_pixel_count(size, pixels.size());
}

bool MutableBitmapView::is_valid() const
{
    return has_matching_pixel_count(size, pixels.size());
}

BitmapView MutableBitmapView::as_read_only() const
{
    return BitmapView{
        .size = size,
        .pixels = pixels,
    };
}

}  // namespace display
