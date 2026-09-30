#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "font.hpp"

namespace display {

enum class FontLoadStatus : uint8_t {
    Ok = 0,
    BitmapLoadFailed,
    InvalidBitmap,
    UnknownFont,
};

// Built-in bitmaps. Add a font by appending an enumerator, declaring its config
// and pixel array, appending references to both in builtin_fonts_, and
// increasing kBuiltinFontCount.
enum class FontId : uint8_t {
    English7x3 = 0,
};

// Owns every built-in bitmap and the Font view over it. The singleton outlives
// every class that shares font().
class FontManager final {
public:
    static FontManager& instance();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // Loads every built-in bitmap. A bitmap that is already loaded is skipped.
    // Returns Ok when every bitmap is loaded. When one fails, the remaining
    // bitmaps are still loaded and the first failure is returned, so a later
    // call retries only the bitmaps that failed.
    [[nodiscard]] FontLoadStatus load();

    // Loads one bitmap. A successful load is kept; a later call returns Ok
    // without reading the file again. A failed load can be retried.
    [[nodiscard]] FontLoadStatus load(FontId id);

    [[nodiscard]] bool is_loaded(FontId id) const;
    [[nodiscard]] const Font& font(FontId id) const;

private:
    // config and pixels refer to this manager's members for that font.
    struct ManagedFont {
        const FontConfig& config;
        std::span<RgbColor> pixels;
        Font font;
        bool loaded;
    };

    static constexpr FontConfig english_7x3_config_ = configure_font<3, 7>(
        "/assets/font_en_7x3.bmp", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZc!?:.,-+\xB0 ");

    FontManager() = default;

    [[nodiscard]] static bool is_known(FontId id);

    static constexpr std::size_t kBuiltinFontCount = 1;

    std::array<RgbColor, english_7x3_config_.pixel_count()> english_7x3_pixels_{};

    // Indexed by FontId.
    std::array<ManagedFont, kBuiltinFontCount> builtin_fonts_{{
        {
            .config = english_7x3_config_,
            .pixels = english_7x3_pixels_,
            .font = {},
            .loaded = false,
        },
    }};
};

}  // namespace display
