#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

#include "bitmap_file.hpp"
#include "geometry.hpp"

namespace display {

// Fixed gap between vertically stacked characters in a font bitmap.
inline constexpr uint16_t kBitmapCharacterSeparatorPixels = 1;

struct FontConfig {
    Size character_size{};
    std::string_view character_lookup{};
    const char* bitmap_path{};

    [[nodiscard]] constexpr Size bitmap_size() const
    {
        if (character_size.width == 0 || character_size.height == 0 || character_lookup.empty()) {
            return {};
        }

        const uint64_t character_count = character_lookup.size();
        const uint64_t bitmap_height = character_count * character_size.height
            + (character_count - 1) * kBitmapCharacterSeparatorPixels;
        if (bitmap_height > std::numeric_limits<uint16_t>::max()) {
            return {};
        }

        return {
            .width = character_size.width,
            .height = static_cast<uint16_t>(bitmap_height),
        };
    }

    [[nodiscard]] constexpr std::size_t pixel_count() const
    {
        const Size size = bitmap_size();
        return static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height);
    }
};

template <uint16_t CharacterWidth, uint16_t CharacterHeight>
[[nodiscard]] constexpr FontConfig configure_font(const char* bitmap_path,
                                                  std::string_view character_lookup)
{
    return {
        .character_size = {.width = CharacterWidth, .height = CharacterHeight},
        .character_lookup = character_lookup,
        .bitmap_path = bitmap_path,
    };
}

struct Font {
    FontConfig config{};
    BitmapFile bitmap{};

    // Verifies that the font bitmap matches the font config specifications.
    [[nodiscard]] bool is_valid() const;
};

}  // namespace display
