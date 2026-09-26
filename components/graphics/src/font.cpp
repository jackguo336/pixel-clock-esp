#include "font.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace display {

bool Font::is_valid() const
{
    const uint16_t character_width = config.character_size.width;
    const uint16_t character_height = config.character_size.height;
    if (character_width == 0 || character_height == 0 || config.character_lookup.empty()) {
        return false;
    }
    if (!bitmap.is_valid() || bitmap.size.width != character_width) {
        return false;
    }

    const uint64_t lookup_count = config.character_lookup.size();
    const uint64_t atlas_height = lookup_count * character_height
        + (lookup_count - 1) * kAtlasRowSeparatorPixels;
    constexpr uint64_t kMaxExtent = std::numeric_limits<uint16_t>::max();
    if (atlas_height > kMaxExtent) {
        return false;
    }
    return bitmap.size.height == static_cast<uint16_t>(atlas_height);
}

}  // namespace display
