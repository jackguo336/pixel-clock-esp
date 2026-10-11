#include "text_measurement.hpp"

#include <cstdint>
#include <limits>

namespace graphics {

Size measure_text(const Font& font, std::string_view text)
{
    if (!font.is_valid() || text.empty()) {
        return {};
    }

    const uint64_t character_count = text.size();
    const uint64_t character_width = font.config.character_size.width;
    const uint64_t width = character_count * character_width
        + (character_count - 1) * static_cast<uint64_t>(kInterCharacterGapPixels);
    constexpr uint64_t kMaxExtent = std::numeric_limits<uint16_t>::max();
    return Size{
        .width = static_cast<uint16_t>(width > kMaxExtent ? kMaxExtent : width),
        .height = font.config.character_size.height,
    };
}

}  // namespace graphics
