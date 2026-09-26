#pragma once

#include <cstdint>
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
};

// Non-owning atlas. The config views and bitmap pixels must outlive the Font.
struct Font {
    FontConfig config{};
    BitmapFile bitmap{};

    // True when the atlas is one character wide and stacks one glyph row per
    // lookup character, with a one-pixel separator between rows.
    [[nodiscard]] bool is_valid() const;
};

}  // namespace display
