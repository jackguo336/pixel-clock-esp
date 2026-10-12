#include <array>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>
#include <variant>

#include "element_tree.hpp"
#include "elements.hpp"
#include "views/text_carousel_view.hpp"
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

views::TextCarouselView make_carousel(const graphics::Font* font)
{
    return views::TextCarouselView{kTexts, font, kSlotSize};
}

bool build_at(const views::TextCarouselView& view, std::chrono::microseconds timestamp,
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

}  // namespace

TEST(TextCarouselView, BuildsViewportStripAndLoopingSlots)
{
    const graphics::Font font{};
    const views::TextCarouselView view = make_carousel(&font);
    std::array<graphics::ElementTreeNode, kCarouselNodeCount> storage{};
    graphics::ElementTree tree{};

    ASSERT_TRUE(build_at(view, std::chrono::milliseconds{0}, storage, tree));

    ASSERT_EQ(tree.nodes.size(), kCarouselNodeCount);
    EXPECT_EQ(tree.root, 0);

    const graphics::ContainerElementPayload* viewport = container_payload(tree.nodes[0].element);
    ASSERT_NE(viewport, nullptr);
    EXPECT_EQ(viewport->layout_system, graphics::LayoutSystem::ChildDefinedPositions);
    ASSERT_TRUE(viewport->fixed_size.has_value());
    EXPECT_EQ(viewport->fixed_size->width, kSlotSize.width);
    EXPECT_EQ(viewport->fixed_size->height, kSlotSize.height);
    ASSERT_TRUE(tree.nodes[0].first_child.has_value());
    EXPECT_EQ(*tree.nodes[0].first_child, 1);

    const graphics::ContainerElementPayload* strip = container_payload(tree.nodes[1].element);
    ASSERT_NE(strip, nullptr);
    EXPECT_EQ(strip->layout_system, graphics::LayoutSystem::Stacked);
    EXPECT_EQ(strip->layout_direction, graphics::StackDirection::TopToBottom);
    EXPECT_FALSE(strip->fixed_size.has_value());
    EXPECT_EQ(tree.nodes[1].element.position.x, 0);
    EXPECT_EQ(tree.nodes[1].element.position.y, 0);
    ASSERT_TRUE(tree.nodes[1].first_child.has_value());
    EXPECT_EQ(*tree.nodes[1].first_child, 2);

    constexpr std::string_view kExpectedTexts[]{"A", "B", "C", "D", "A"};
    for (std::size_t slot = 0; slot < kSlotCount; ++slot) {
        const std::size_t slot_index = 2 + slot * 2;
        const std::size_t text_index = slot_index + 1;

        const graphics::ContainerElementPayload* slot_payload = container_payload(tree.nodes[slot_index].element);
        ASSERT_NE(slot_payload, nullptr) << "slot " << slot;
        ASSERT_TRUE(slot_payload->fixed_size.has_value()) << "slot " << slot;
        EXPECT_EQ(slot_payload->fixed_size->width, kSlotSize.width) << "slot " << slot;
        EXPECT_EQ(slot_payload->fixed_size->height, kSlotSize.height) << "slot " << slot;
        ASSERT_TRUE(tree.nodes[slot_index].first_child.has_value()) << "slot " << slot;
        EXPECT_EQ(*tree.nodes[slot_index].first_child, static_cast<graphics::ElementNodeIndex>(text_index));

        const graphics::TextElementPayload* letter = text_payload(tree.nodes[text_index].element);
        ASSERT_NE(letter, nullptr) << "slot " << slot;
        EXPECT_EQ(letter->text, kExpectedTexts[slot]) << "slot " << slot;
        EXPECT_EQ(letter->font, &font) << "slot " << slot;

        if (slot + 1 < kSlotCount) {
            ASSERT_TRUE(tree.nodes[slot_index].next_sibling.has_value()) << "slot " << slot;
            EXPECT_EQ(*tree.nodes[slot_index].next_sibling,
                      static_cast<graphics::ElementNodeIndex>(slot_index + 2));
        } else {
            EXPECT_FALSE(tree.nodes[slot_index].next_sibling.has_value());
        }
        EXPECT_FALSE(tree.nodes[text_index].next_sibling.has_value());
    }
}

TEST(TextCarouselView, HoldsTheFirstLetterDuringTheInitialPause)
{
    const views::TextCarouselView view = make_carousel(nullptr);
    const auto pause = views::TextCarouselView::kPauseDuration;

    EXPECT_EQ(view.scroll_offset(std::chrono::milliseconds{0}), 0);
    EXPECT_EQ(view.scroll_offset(pause - std::chrono::milliseconds{1}), 0);
}

TEST(TextCarouselView, StepsOnePixelEveryStepDuration)
{
    const views::TextCarouselView view = make_carousel(nullptr);
    const auto pause = views::TextCarouselView::kPauseDuration;
    const auto step = views::TextCarouselView::kStepDuration;

    EXPECT_EQ(view.scroll_offset(pause), 1);
    EXPECT_EQ(view.scroll_offset(pause + step), 2);
    EXPECT_EQ(view.scroll_offset(pause + 6 * step - std::chrono::milliseconds{1}), 6);
    EXPECT_EQ(view.scroll_offset(pause + 6 * step), 7);
}

TEST(TextCarouselView, RestsOnTheNextLetterForThePause)
{
    const views::TextCarouselView view = make_carousel(nullptr);
    const auto pause = views::TextCarouselView::kPauseDuration;
    const auto transition = pause + (kSlotSize.height - 1) * views::TextCarouselView::kStepDuration;

    EXPECT_EQ(view.scroll_offset(transition), static_cast<int16_t>(kSlotSize.height));
    EXPECT_EQ(view.scroll_offset(transition + pause - std::chrono::milliseconds{1}),
              static_cast<int16_t>(kSlotSize.height));
    EXPECT_EQ(view.scroll_offset(transition + pause), static_cast<int16_t>(kSlotSize.height + 1));
}

TEST(TextCarouselView, WrapsFromTheLoopCopyBackToTheFirstLetter)
{
    const views::TextCarouselView view = make_carousel(nullptr);
    const auto transition = views::TextCarouselView::kPauseDuration
        + (kSlotSize.height - 1) * views::TextCarouselView::kStepDuration;
    const auto loop = kTextCount * transition;
    const auto last_offset = static_cast<int16_t>(kTextCount * kSlotSize.height - 1);

    EXPECT_EQ(view.scroll_offset(loop - std::chrono::milliseconds{1}), last_offset);
    EXPECT_EQ(view.scroll_offset(loop), 0);
    EXPECT_EQ(view.scroll_offset(loop + transition), static_cast<int16_t>(kSlotSize.height));
}

TEST(TextCarouselView, LayoutFollowsTheScrollOffset)
{
    const auto kTimestamp = views::TextCarouselView::kPauseDuration + views::TextCarouselView::kStepDuration;
    const views::TextCarouselView view = make_carousel(nullptr);
    const int16_t offset = view.scroll_offset(kTimestamp);
    ASSERT_EQ(offset, 2);

    std::array<graphics::ElementTreeNode, kCarouselNodeCount> storage{};
    graphics::ElementTree tree{};
    ASSERT_TRUE(build_at(view, kTimestamp, storage, tree));
    ASSERT_EQ(tree.nodes.size(), kCarouselNodeCount);

    EXPECT_EQ(tree.nodes[1].layout.origin_on_canvas.y, static_cast<int16_t>(-offset));
    for (int16_t slot = 0; slot < static_cast<int16_t>(kSlotCount); ++slot) {
        const std::size_t slot_index = static_cast<std::size_t>(2 + slot * 2);
        const int16_t expected_y = static_cast<int16_t>(slot * kSlotSize.height - offset);
        EXPECT_EQ(tree.nodes[slot_index].layout.origin_on_canvas.x, 0) << "slot " << slot;
        EXPECT_EQ(tree.nodes[slot_index].layout.origin_on_canvas.y, expected_y) << "slot " << slot;
    }
}

TEST(TextCarouselView, PlacesViewportAfterAOnePixelSpacer)
{
    const views::TextCarouselView view = make_carousel(nullptr);
    const views::ViewData view_data{};
    std::array<graphics::ElementTreeNode, 16> storage{};
    graphics::ElementTreeBuilder builder{storage};

    const graphics::Element row{
        .payload = graphics::ContainerElementPayload{
            .layout_direction = graphics::StackDirection::LeftToRight,
            .layout_system = graphics::LayoutSystem::Stacked,
        },
    };
    ASSERT_TRUE(builder.add_container(row, [&](auto& children) {
        const graphics::Element leading{
            .payload = graphics::FilledRectangleElementPayload{
                .size = {.width = kLeadingWidth, .height = kSlotSize.height},
            },
        };
        if (!children.add_terminal(leading)) {
            return;
        }

        const graphics::Element spacer{
            .payload = graphics::ContainerElementPayload{
                .fixed_size = graphics::Size{.width = 1, .height = 0},
            },
        };
        if (!children.add_container(spacer, [](auto&) {})) {
            return;
        }

        if (!view.build(view_data, children)) {
            return;
        }
    }));

    const graphics::ElementTree tree = builder.build();
    ASSERT_GE(tree.nodes.size(), 4u);

    const graphics::ContainerElementPayload* viewport = container_payload(tree.nodes[3].element);
    ASSERT_NE(viewport, nullptr);
    EXPECT_EQ(viewport->layout_system, graphics::LayoutSystem::ChildDefinedPositions);
    EXPECT_EQ(tree.nodes[3].layout.origin_on_canvas.x, static_cast<int16_t>(kLeadingWidth + 1));
    EXPECT_EQ(tree.nodes[3].layout.origin_on_canvas.y, 0);
}

TEST(TextCarouselView, EmptyTextListBuildsWithZeroOffset)
{
    const graphics::Font font{};
    const views::TextCarouselView view{std::span<const std::string_view>{}, &font, kSlotSize};

    EXPECT_EQ(view.scroll_offset(std::chrono::milliseconds{0}), 0);
    EXPECT_EQ(view.scroll_offset(std::chrono::milliseconds{15000}), 0);

    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTree tree{};
    ASSERT_TRUE(build_at(view, std::chrono::milliseconds{15000}, storage, tree));
    ASSERT_EQ(tree.nodes.size(), 2u);
    EXPECT_NE(container_payload(tree.nodes[0].element), nullptr);
    EXPECT_NE(container_payload(tree.nodes[1].element), nullptr);
    EXPECT_EQ(text_payload(tree.nodes[0].element), nullptr);
    EXPECT_EQ(text_payload(tree.nodes[1].element), nullptr);
}

TEST(TextCarouselView, BuildFailsWhenNodeStorageIsExhausted)
{
    const views::TextCarouselView view = make_carousel(nullptr);
    std::array<graphics::ElementTreeNode, kCarouselNodeCount - 1> storage{};
    graphics::ElementTreeBuilder builder{storage};
    const views::ViewData view_data{};

    EXPECT_FALSE(view.build(view_data, builder));
}
