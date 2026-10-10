#include <array>
#include <string_view>

#include "bitmap_file.hpp"
#include "element_tree.hpp"
#include "elements.hpp"
#include "font.hpp"
#include "gtest/gtest.h"
#include "text_measurement.hpp"
#include "utils.hpp"

using graphics_test::expect_size;
using graphics_test::make_font;

namespace {

void expect_layout(graphics::ElementLayout layout, int16_t x, int16_t y, uint16_t width, uint16_t height)
{
    EXPECT_EQ(layout.origin_on_canvas.x, x);
    EXPECT_EQ(layout.origin_on_canvas.y, y);
    EXPECT_EQ(layout.size.width, width);
    EXPECT_EQ(layout.size.height, height);
}

graphics::Element make_container(graphics::ElementId id, graphics::Position position,
                                 graphics::StackDirection layout_direction,
                                 graphics::LayoutSystem layout_system = graphics::LayoutSystem::ChildDefinedPositions)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .payload = graphics::ContainerElementPayload{
            .layout_direction = layout_direction,
            .layout_system = layout_system,
        },
    };
}

graphics::Element make_bitmap(graphics::ElementId id, graphics::Position position, const graphics::BitmapFile* bitmap)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .payload = graphics::BitmapElementPayload{.bitmap = bitmap},
    };
}

graphics::Element make_text(graphics::ElementId id, graphics::Position position, std::string_view text,
                            const graphics::Font* font)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .payload = graphics::TextElementPayload{.text = text, .font = font},
    };
}

graphics::Element make_rectangle(graphics::ElementId id, graphics::Position position, graphics::Size size)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .payload = graphics::FilledRectangleElementPayload{.size = size},
    };
}

}  // namespace

TEST(ElementLayout, RectangleOriginIsRootPositionPlusItsOwnPosition)
{
    std::array<graphics::ElementTreeNode, 3> storage{};
    storage[2].layout = graphics::ElementLayout{
        .origin_on_canvas = {.x = -1, .y = -2},
        .size = {.width = 9, .height = 8},
    };
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, graphics::StackDirection::LeftToRight),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 2}, {.x = 3, .y = 4}, {.width = 5, .height = 6})));
        }));

    const graphics::ElementTree tree = builder.build();

    expect_layout(tree.nodes[0].layout, 2, 1, 8, 10);
    expect_layout(tree.nodes[1].layout, 5, 5, 5, 6);
    expect_layout(storage[2].layout, -1, -2, 9, 8);
}

TEST(ElementLayout, NullBitmapAndInvalidFontTextHaveNoSize)
{
    const graphics::Font invalid_font{};
    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {}, graphics::StackDirection::LeftToRight), [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {}, nullptr)));
            EXPECT_TRUE(children.add_terminal(make_text({.value = 3}, {.x = 1, .y = 0}, "A", nullptr)));
            EXPECT_TRUE(children.add_terminal(make_text({.value = 4}, {.x = 0, .y = 2}, "A", &invalid_font)));
        }));

    const graphics::ElementTree tree = builder.build();

    expect_layout(tree.nodes[1].layout, 0, 0, 0, 0);
    expect_layout(tree.nodes[2].layout, 1, 0, 0, 0);
    expect_layout(tree.nodes[3].layout, 0, 2, 0, 0);
}

TEST(ElementLayout, TextFrameMatchesMeasureText)
{
    const std::array<graphics::RgbColor, 3> pixels{
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
        graphics::RgbColor{},
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
    };
    const graphics::Font font =
        make_font({.width = 1, .height = 1}, "AB", pixels, {.width = 1, .height = 3}, graphics_test::kBitmapPath);
    ASSERT_TRUE(font.is_valid());

    std::array<graphics::ElementTreeNode, 1> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_terminal(make_text({.value = 1}, {.x = 1, .y = 2}, "AB", &font)));

    const graphics::ElementTree tree = builder.build();

    const graphics::Size measured = graphics::measure_text(font, "AB");
    expect_size(measured, 3, 1);
    expect_layout(tree.nodes[0].layout, 1, 2, measured.width, measured.height);
}

TEST(ElementLayout, ChildDefinedPositionsAreOffsetsFromTheContainerOrigin)
{
    std::array<graphics::ElementTreeNode, 2> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 3}, graphics::StackDirection::TopToBottom),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 2}, {.x = 4, .y = 1}, {.width = 2, .height = 2})));
        }));

    const graphics::ElementTree tree = builder.build();

    expect_layout(tree.nodes[0].layout, 2, 3, 6, 3);
    expect_layout(tree.nodes[1].layout, 6, 4, 2, 2);
}

TEST(ElementLayout, StackedLeftToRightIgnoresStoredPositions)
{
    const std::array<graphics::RgbColor, 2> first_pixels{};
    const std::array<graphics::RgbColor, 1> second_pixels{};
    const graphics::BitmapFile first_bitmap{.size = {.width = 2, .height = 1}, .pixels = first_pixels};
    const graphics::BitmapFile second_bitmap{.size = {.width = 1, .height = 1}, .pixels = second_pixels};

    std::array<graphics::ElementTreeNode, 3> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 2}, graphics::StackDirection::LeftToRight,
                       graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 7, .y = 7}, &first_bitmap)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, {.x = 4, .y = 4}, &second_bitmap)));
        }));

    const graphics::ElementTree tree = builder.build();

    expect_layout(tree.nodes[0].layout, 1, 2, 3, 1);
    expect_layout(tree.nodes[1].layout, 1, 2, 2, 1);
    expect_layout(tree.nodes[2].layout, 3, 2, 1, 1);
}

TEST(ElementLayout, StackedTopToBottomIgnoresStoredPositions)
{
    const std::array<graphics::RgbColor, 2> first_pixels{};
    const std::array<graphics::RgbColor, 1> second_pixels{};
    const graphics::BitmapFile first_bitmap{.size = {.width = 1, .height = 2}, .pixels = first_pixels};
    const graphics::BitmapFile second_bitmap{.size = {.width = 1, .height = 1}, .pixels = second_pixels};

    std::array<graphics::ElementTreeNode, 3> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, graphics::StackDirection::TopToBottom,
                       graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 5, .y = 6}, &first_bitmap)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, {.x = 8, .y = 9}, &second_bitmap)));
        }));

    const graphics::ElementTree tree = builder.build();

    expect_layout(tree.nodes[0].layout, 2, 1, 1, 3);
    expect_layout(tree.nodes[1].layout, 2, 1, 1, 2);
    expect_layout(tree.nodes[2].layout, 2, 3, 1, 1);
}

TEST(ElementLayout, ContainerSizeUsesTheFarthestChildAndAdvancesByNestedSize)
{
    const std::array<graphics::RgbColor, 1> short_pixels{};
    const std::array<graphics::RgbColor, 2> tall_pixels{};
    const std::array<graphics::RgbColor, 1> following_pixels{};
    const graphics::BitmapFile short_bitmap{.size = {.width = 1, .height = 1}, .pixels = short_pixels};
    const graphics::BitmapFile tall_bitmap{.size = {.width = 1, .height = 2}, .pixels = tall_pixels};
    const graphics::BitmapFile following_bitmap{.size = {.width = 1, .height = 1}, .pixels = following_pixels};

    std::array<graphics::ElementTreeNode, 5> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {}, graphics::StackDirection::TopToBottom, graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, {.x = 4, .y = 4}, graphics::StackDirection::LeftToRight,
                               graphics::LayoutSystem::Stacked),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 3}, {.x = 9, .y = 9}, &short_bitmap)));
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 4}, {.x = 8, .y = 8}, &tall_bitmap)));
                }));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 5}, {.x = 3, .y = 3}, &following_bitmap)));
        }));

    const graphics::ElementTree tree = builder.build();

    expect_layout(tree.nodes[2].layout, 0, 0, 1, 1);
    expect_layout(tree.nodes[3].layout, 1, 0, 1, 2);
    expect_layout(tree.nodes[1].layout, 0, 0, 2, 2);
    expect_layout(tree.nodes[4].layout, 0, 2, 1, 1);
    expect_layout(tree.nodes[0].layout, 0, 0, 2, 3);
}

TEST(ElementLayout, ReusingNodeStorageReplacesPreviousLayouts)
{
    std::array<graphics::ElementTreeNode, 2> storage{};

    {
        graphics::ElementTreeBuilder builder{storage};
        ASSERT_TRUE(builder.add_container(
            make_container({.value = 1}, {.x = 2, .y = 3}, graphics::StackDirection::LeftToRight),
            [&](auto& children) {
                EXPECT_TRUE(children.add_terminal(
                    make_rectangle({.value = 2}, {.x = 4, .y = 5}, {.width = 6, .height = 7})));
            }));

        const graphics::ElementTree tree = builder.build();
        expect_layout(tree.nodes[0].layout, 2, 3, 10, 12);
        expect_layout(tree.nodes[1].layout, 6, 8, 6, 7);
    }

    {
        graphics::ElementTreeBuilder builder{storage};
        ASSERT_TRUE(builder.add_terminal(make_bitmap({.value = 3}, {.x = 1, .y = 0}, nullptr)));

        const graphics::ElementTree tree = builder.build();
        ASSERT_EQ(tree.nodes.size(), 1u);
        expect_layout(tree.nodes[0].layout, 1, 0, 0, 0);
    }
}
