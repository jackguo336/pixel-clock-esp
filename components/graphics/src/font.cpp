#include "font.hpp"

namespace display {

bool Font::is_valid() const
{
    const Size expected_size = config.bitmap_size();
    if (expected_size.width == 0 || expected_size.height == 0) {
        return false;
    }
    return bitmap.is_valid() && bitmap.size.width == expected_size.width
        && bitmap.size.height == expected_size.height;
}

}  // namespace display
