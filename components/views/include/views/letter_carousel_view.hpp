#pragma once

#include <chrono>
#include <cstdint>
#include <span>
#include <string_view>

#include "font.hpp"
#include "geometry.hpp"
#include "views/view.hpp"

namespace views {

// Vertically scrolls a strip of text slots. The offset is a pure function of
// ViewData::timestamp, so the view holds no playback state.
class LetterCarouselView final : public View {
public:
    static constexpr std::chrono::milliseconds kPauseDuration{2000};
    static constexpr std::chrono::milliseconds kStepDuration{50};

    // texts and font must outlive any tree built from this view.
    LetterCarouselView(std::span<const std::string_view> texts, const graphics::Font* font, graphics::Size slot_size);

    [[nodiscard]] bool build(const ViewData& view_data,
                             graphics::ElementTreeBuilder& builder) const override;

private:
    // Pixels the strip is shifted up at the given time, in [0, texts.size() * slot_size.height).
    [[nodiscard]] int16_t automatic_scroll_offset(std::chrono::microseconds timestamp) const;

    std::span<const std::string_view> texts_;
    const graphics::Font* font_;
    graphics::Size slot_size_;
};

}  // namespace views
