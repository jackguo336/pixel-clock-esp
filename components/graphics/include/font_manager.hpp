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

// Supported fonts
enum class FontId : uint8_t {
    English7x3 = 0,
};

class FontManager final {
public:
    static FontManager& instance();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // Loads all supported fonts.
    [[nodiscard]] FontLoadStatus load();

    [[nodiscard]] FontLoadStatus load(FontId id);

    [[nodiscard]] bool is_loaded(FontId id) const;
    [[nodiscard]] const Font& font(FontId id) const;

private:
    struct ManagedFont {
        const FontConfig& config;
        // A reference to the font's bitmap pixels owned by the manager.
        std::span<RgbColor> pixels;
        Font font;
        bool loaded;
    };

    static constexpr FontConfig english_7x3_config_ = configure_font<3, 7>(
        "/assets/font_en_7x3.bmp", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZc!?:.,-+\xB0 ");

    FontManager() = default;

    [[nodiscard]] static bool is_known(FontId id);

    // Number of supported fonts.
    static constexpr std::size_t kFontCount = 1;

    std::array<RgbColor, english_7x3_config_.pixel_count()> english_7x3_pixels_{};

    // Allow font configuration and pixel storage to be indexed by FontId.
    std::array<ManagedFont, kFontCount> fonts_{{
        {
            .config = english_7x3_config_,
            .pixels = english_7x3_pixels_,
            .font = {},
            .loaded = false,
        },
    }};
};

}  // namespace display
