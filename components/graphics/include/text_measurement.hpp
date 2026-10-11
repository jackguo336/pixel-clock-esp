#pragma once

#include <cstdint>
#include <string_view>

#include "font.hpp"
#include "geometry.hpp"

namespace graphics {

// Blank columns left between adjacent characters when measuring and drawing a string.
// Prevents characters from touching each other.
inline constexpr int32_t kInterCharacterGapPixels = 1;

[[nodiscard]] Size measure_text(const Font& font, std::string_view text);

}  // namespace graphics
