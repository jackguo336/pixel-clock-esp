#include "views/letter_carousel_view.hpp"

#include "elements.hpp"

namespace views {
namespace {

// One glyph, top-aligned in a fixed slot. The slot height is the scroll pitch,
// so a shorter glyph leaves an empty row instead of letting the next glyph peek.
bool add_letter_slot(graphics::ElementTreeBuilder& builder, std::string_view text, const graphics::Font* font,
                     graphics::Size slot_size)
{
    const graphics::Element slot{
        .payload = graphics::ContainerElementPayload{
            .layout_direction = graphics::StackDirection::TopToBottom,
            .layout_system = graphics::LayoutSystem::Stacked,
            .fixed_size = slot_size,
        },
    };
    return builder.add_container(slot, [&](graphics::ElementTreeBuilder& slot_children) {
        const graphics::Element letter{
            .payload = graphics::TextElementPayload{
                .text = text,
                .font = font,
            },
        };
        if (!slot_children.add_terminal(letter)) {
            return;
        }
    });
}

}  // namespace

LetterCarouselView::LetterCarouselView(std::span<const std::string_view> texts, const graphics::Font* font,
                                       graphics::Size slot_size)
    : texts_{texts}
    , font_{font}
    , slot_size_{slot_size}
{
}

bool LetterCarouselView::build(const ViewData& view_data, graphics::ElementTreeBuilder& builder) const
{
    const int16_t offset = automatic_scroll_offset(view_data.timestamp);

    // ChildDefinedPositions honors the scrolling text slots' y. A stacked viewport would ignore it.
    const graphics::Element viewport{
        .payload = graphics::ContainerElementPayload{
            .layout_direction = graphics::StackDirection::TopToBottom,
            .layout_system = graphics::LayoutSystem::ChildDefinedPositions,
            .fixed_size = slot_size_,
        },
    };
    return builder.add_container(viewport, [&](graphics::ElementTreeBuilder& viewport_children) {
        const graphics::Element scrolling_text_slots{
            .position = {.x = 0, .y = static_cast<int16_t>(-static_cast<int32_t>(offset))},
            .payload = graphics::ContainerElementPayload{
                .layout_direction = graphics::StackDirection::TopToBottom,
                .layout_system = graphics::LayoutSystem::Stacked,
            },
        };
        if (!viewport_children.add_container(
                scrolling_text_slots, [&](graphics::ElementTreeBuilder& scrolling_text_slot_children) {
                    for (const std::string_view text : texts_) {
                        if (!add_letter_slot(scrolling_text_slot_children, text, font_, slot_size_)) {
                            return;
                        }
                    }
                    // Repeat the first letter so the last scroll shows the same glyph as offset 0.
                    if (!texts_.empty() &&
                        !add_letter_slot(scrolling_text_slot_children, texts_[0], font_, slot_size_)) {
                        return;
                    }
                })) {
            return;
        }
    });
}

int16_t LetterCarouselView::automatic_scroll_offset(std::chrono::microseconds timestamp) const
{
    const int32_t slot_height_pixels = slot_size_.height;
    const int32_t transition_count = static_cast<int32_t>(texts_.size());
    if (transition_count == 0 || slot_height_pixels == 0) {
        return 0;
    }

    // Use slot_height_pixels - 1 because the final pixel step of a transition is the first 
    // instant of the next transition's pause and should not be double counted.
    const std::chrono::microseconds transition_duration =
        kPauseDuration + (slot_height_pixels - 1) * kStepDuration;
    const std::chrono::microseconds loop_duration = transition_count * transition_duration;

    const std::chrono::microseconds time_in_loop = timestamp % loop_duration;
    const int32_t completed_transition_count = static_cast<int32_t>(time_in_loop / transition_duration);
    const std::chrono::microseconds time_in_transition = time_in_loop % transition_duration;

    int32_t pixels_scrolled_in_transition = 0;
    if (time_in_transition >= kPauseDuration) {
        const std::chrono::microseconds time_scrolling = time_in_transition - kPauseDuration;
        pixels_scrolled_in_transition = static_cast<int32_t>(time_scrolling / kStepDuration) + 1;
    }

    return static_cast<int16_t>(completed_transition_count * slot_height_pixels + pixels_scrolled_in_transition);
}

}  // namespace views
