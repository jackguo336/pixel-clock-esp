#include <array>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>
#include <variant>

#include "element_tree.hpp"
#include "elements.hpp"
#include "views/letter_carousel_view.hpp"
#include "gtest/gtest.h"

namespace {

constexpr graphics::Size kSlotSize{.width = 3, .height = 8};
constexpr const std::string_view kTexts[]{"A", "B", "C", "D"};
constexpr std::size_t kTextCount = std::size(kTexts);
constexpr std::size_t kSlotCount = kTextCount + 1;
// Viewport, strip, one container and one glyph per slot (including the loop copy).
constexpr std::size_t kCarouselNodeCount = 2 + kSlotCount * 2;
constexpr uint16_t kLeadingWidth = 11;

const graphics::ContainerElementPayload* container_payload(const graphics::Element& element)
{
    return std::get_if<graphics::ContainerElementPayload>(&element.payload);
}

const graphics::TextElementPayload* text_payload(const graphics::Element& element)
{
    return std::get_if<graphics::TextElementPayload>(&element.payload);
}

views::LetterCarouselView make_carousel(const graphics::Font* font)
{
    return views::LetterCarouselView{kTexts, font, kSlotSize};
}

bool build_at(const views::LetterCarouselView& view, std::chrono::microseconds timestamp,
              std::span<graphics::ElementTreeNode> storage, graphics::ElementTree& tree)
{
    graphics::ElementTreeBuilder builder{storage};
    const views::ViewData view_data{.timestamp = timestamp};
    if (!view.build(view_data, builder)) {
        return false;
    }
    tree = builder.build();
    return true;
}

void expect_strip_shifted_up(const views::LetterCarouselView& view, std::chrono::microseconds timestamp,
                             int16_t pixels)
{
    std::array<graphics::ElementTreeNode, kCarouselNodeCount> storage{};
    graphics::ElementTree tree{};
    ASSERT_TRUE(build_at(view, timestamp, storage, tree));
    ASSERT_EQ(tree.nodes.size(), kCarouselNodeCount);
    EXPECT_EQ(tree.nodes[1].layout.origin_on_canvas.y, static_cast<int16_t>(-pixels));
}

}  // namespace

TEST(LetterCarouselView, HoldsTheFirstLetterDuringTheInitialPause)
{
    const views::LetterCarouselView view = make_carousel(nullptr);
    const auto pause = views::LetterCarouselView::kPauseDuration;

    expect_strip_shifted_up(view, std::chrono::milliseconds{0}, 0);
    expect_strip_shifted_up(view, pause - std::chrono::milliseconds{1}, 0);
}

TEST(LetterCarouselView, StepsOnePixelEveryStepDuration)
{
    const views::LetterCarouselView view = make_carousel(nullptr);
    const auto pause = views::LetterCarouselView::kPauseDuration;
    const auto step = views::LetterCarouselView::kStepDuration;

    expect_strip_shifted_up(view, pause, 1);
    expect_strip_shifted_up(view, pause + step, 2);
    expect_strip_shifted_up(view, pause + 6 * step - std::chrono::milliseconds{1}, 6);
    expect_strip_shifted_up(view, pause + 6 * step, 7);
}

TEST(LetterCarouselView, RestsOnTheNextLetterForThePause)
{
    const views::LetterCarouselView view = make_carousel(nullptr);
    const auto pause = views::LetterCarouselView::kPauseDuration;
    const auto transition = pause + (kSlotSize.height - 1) * views::LetterCarouselView::kStepDuration;

    expect_strip_shifted_up(view, transition, static_cast<int16_t>(kSlotSize.height));
    expect_strip_shifted_up(view, transition + pause - std::chrono::milliseconds{1},
                            static_cast<int16_t>(kSlotSize.height));
    expect_strip_shifted_up(view, transition + pause, static_cast<int16_t>(kSlotSize.height + 1));
}

TEST(LetterCarouselView, WrapsFromTheLoopCopyBackToTheFirstLetter)
{
    const views::LetterCarouselView view = make_carousel(nullptr);
    const auto transition = views::LetterCarouselView::kPauseDuration
        + (kSlotSize.height - 1) * views::LetterCarouselView::kStepDuration;
    const auto loop = kTextCount * transition;
    const auto last_offset = static_cast<int16_t>(kTextCount * kSlotSize.height - 1);

    expect_strip_shifted_up(view, loop - std::chrono::milliseconds{1}, last_offset);
    expect_strip_shifted_up(view, loop, 0);
    expect_strip_shifted_up(view, loop + transition, static_cast<int16_t>(kSlotSize.height));
}

TEST(LetterCarouselView, LayoutFollowsTheScrollOffset)
{
    const auto kTimestamp = views::LetterCarouselView::kPauseDuration + views::LetterCarouselView::kStepDuration;
    const views::LetterCarouselView view = make_carousel(nullptr);
    // The pause boundary is the first scrolled pixel; one more step makes two.
    constexpr int16_t kScrolledPixels = 2;

    std::array<graphics::ElementTreeNode, kCarouselNodeCount> storage{};
    graphics::ElementTree tree{};
    ASSERT_TRUE(build_at(view, kTimestamp, storage, tree));
    ASSERT_EQ(tree.nodes.size(), kCarouselNodeCount);

    EXPECT_EQ(tree.nodes[1].layout.origin_on_canvas.y, static_cast<int16_t>(-kScrolledPixels));
    for (int16_t slot = 0; slot < static_cast<int16_t>(kSlotCount); ++slot) {
        const std::size_t slot_index = static_cast<std::size_t>(2 + slot * 2);
        const int16_t expected_y = static_cast<int16_t>(slot * kSlotSize.height - kScrolledPixels);
        EXPECT_EQ(tree.nodes[slot_index].layout.origin_on_canvas.x, 0) << "slot " << slot;
        EXPECT_EQ(tree.nodes[slot_index].layout.origin_on_canvas.y, expected_y) << "slot " << slot;
    }
}
