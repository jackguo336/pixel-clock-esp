#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

#include "bitmap_file.hpp"
#include "geometry.hpp"

namespace display {

// Fixed gap between vertically stacked glyph rows in a font atlas.
inline constexpr uint16_t kAtlasRowSeparatorPixels = 1;

struct FontConfig {
    Size character_size{};
    std::string_view character_lookup{};
    // Optional asset path. Glyph rendering reads bitmap, not this path.
    const char* bitmap_path{};

    [[nodiscard]] constexpr Size bitmap_size() const
    {
        if (character_size.width == 0 || character_size.height == 0 || character_lookup.empty()) {
            return {};
        }

        const uint64_t glyph_rows = character_lookup.size();
        const uint64_t atlas_height = glyph_rows * character_size.height
            + (glyph_rows - 1) * kAtlasRowSeparatorPixels;
        if (atlas_height > std::numeric_limits<uint16_t>::max()) {
            return {};
        }

        return {
            .width = character_size.width,
            .height = static_cast<uint16_t>(atlas_height),
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

// Non-owning atlas. The config views and bitmap pixels must outlive the Font.
struct Font {
    FontConfig config{};
    BitmapFile bitmap{};

    // True when the atlas is one character wide and stacks one glyph row per
    // lookup character, with a one-pixel separator between rows.
    [[nodiscard]] bool is_valid() const;
};

}  // namespace display
