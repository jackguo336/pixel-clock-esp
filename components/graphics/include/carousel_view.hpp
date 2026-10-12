#pragma once

#include <chrono>
#include <cstdint>
#include <span>
#include <string_view>

#include "font.hpp"
#include "geometry.hpp"
#include "views.hpp"

namespace graphics {

// Vertically scrolls a strip of text slots. The offset is a pure function of
// ViewData::timestamp, so the view holds no playback state.
class CarouselView final : public View {
public:
    static constexpr std::chrono::milliseconds kPauseDuration{2000};
    static constexpr std::chrono::milliseconds kStepDuration{50};

    // texts and font must outlive any tree built from this view.
    CarouselView(std::span<const std::string_view> texts, const Font* font, Size slot_size);

    [[nodiscard]] bool build(const ViewData& view_data, ElementTreeBuilder& builder) const override;

    // Pixels the strip is shifted up at the given time, in [0, texts.size() * slot_size.height).
    [[nodiscard]] int16_t scroll_offset(std::chrono::microseconds timestamp) const;

private:
    std::span<const std::string_view> texts_;
    const Font* font_;
    Size slot_size_;
};

}  // namespace graphics
