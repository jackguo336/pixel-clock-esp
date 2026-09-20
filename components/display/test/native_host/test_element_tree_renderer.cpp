#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "display/bitmap.hpp"
#include "display/element_tree.hpp"
#include "display/elements.hpp"
#include "element_tree_renderer.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"

namespace {

void expect_rgb_at(const display::LogicalFramebuffer& framebuffer, int32_t x, int32_t y, uint8_t red,
                   uint8_t green, uint8_t blue)
{
    const display::RgbColor* pixel = framebuffer.pixel_at(x, y);
    ASSERT_NE(pixel, nullptr);
    EXPECT_EQ(pixel->red, red);
    EXPECT_EQ(pixel->green, green);
    EXPECT_EQ(pixel->blue, blue);
}

[[nodiscard]] bool framebuffers_equal(const display::LogicalFramebuffer& lhs,
                                      const display::LogicalFramebuffer& rhs)
{
    const auto left = lhs.pixels();
    const auto right = rhs.pixels();
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i].red != right[i].red || left[i].green != right[i].green
            || left[i].blue != right[i].blue) {
            return false;
        }
    }
    return true;
}

display::Element make_container(display::ElementId id, display::Position position,
                                display::LayoutDirection layout_direction)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = display::SolidPaint{},
        .payload = display::ContainerElementPayload{.layout_direction = layout_direction},
    };
}

display::Element make_bitmap(display::ElementId id, display::Position position,
                             const display::BitmapFile* bitmap)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = display::SolidPaint{},
        .payload = display::BitmapElementPayload{.bitmap = bitmap},
    };
}

display::Element make_text(display::ElementId id, display::Position position, std::string_view text)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = display::SolidPaint{},
        .payload = display::TextElementPayload{.text = text},
    };
}

display::Element make_rectangle(display::ElementId id, display::Position position, display::Size size)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = display::SolidPaint{},
        .payload = display::FilledRectangleElementPayload{.size = size},
    };
}

}  // namespace

TEST(ElementTreeRenderer, RendersBitmapRootAtItsOwnPosition)
{
    const std::array<display::RgbColor, 4> pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 255, .blue = 0},
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
        display::RgbColor{.red = 255, .green = 255, .blue = 0},
    };
    const display::BitmapFile bitmap{
        .size = {.width = 2, .height = 2},
        .pixels = pixels,
    };

    std::array<display::ElementTreeNode, 1> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_terminal(make_bitmap({.value = 1}, {.x = 3, .y = 2}, &bitmap)));

    display::LogicalFramebuffer framebuffer;
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 3, 2, 255, 0, 0);
    expect_rgb_at(framebuffer, 4, 2, 0, 255, 0);
    expect_rgb_at(framebuffer, 3, 3, 0, 0, 255);
    expect_rgb_at(framebuffer, 4, 3, 255, 255, 0);
    expect_rgb_at(framebuffer, 2, 2, 0, 0, 0);
    expect_rgb_at(framebuffer, 5, 2, 0, 0, 0);
}

TEST(ElementTreeRenderer, ComposesNestedContainerOrigins)
{
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<display::ElementTreeNode, 3> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, display::LayoutDirection::LeftToRight),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, {.x = 3, .y = 2}, display::LayoutDirection::TopToBottom),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 3}, {.x = 4, .y = 1}, &bitmap)));
                }));
        }));

    display::LogicalFramebuffer framebuffer;
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 9, 4, 9, 8, 7);
    expect_rgb_at(framebuffer, 2, 1, 0, 0, 0);
    expect_rgb_at(framebuffer, 5, 3, 0, 0, 0);
    expect_rgb_at(framebuffer, 4, 1, 0, 0, 0);
}

TEST(ElementTreeRenderer, OverwritesEarlierBitmapsInDeclarationOrder)
{
    const std::array<display::RgbColor, 2> earlier_pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<display::RgbColor, 1> nested_pixels{
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const std::array<display::RgbColor, 1> later_pixels{
        display::RgbColor{.red = 255, .green = 255, .blue = 255},
    };
    const display::BitmapFile earlier_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = earlier_pixels,
    };
    const display::BitmapFile nested_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = nested_pixels,
    };
    const display::BitmapFile later_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = later_pixels,
    };

    std::array<display::ElementTreeNode, 5> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 1}, display::LayoutDirection::TopToBottom),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 3, .y = 2}, &earlier_bitmap)));
            EXPECT_TRUE(children.add_container(
                make_container({.value = 3}, {.x = 2, .y = 1}, display::LayoutDirection::LeftToRight),
                [&](auto& nested) {
                    EXPECT_TRUE(
                        nested.add_terminal(make_bitmap({.value = 4}, {.x = 1, .y = 1}, &nested_bitmap)));
                }));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 5}, {.x = 3, .y = 2}, &later_bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 4, 3, 255, 255, 255);
    expect_rgb_at(framebuffer, 5, 3, 0, 255, 0);
}

TEST(ElementTreeRenderer, LeavesFramebufferUnchangedForEmptyContainer)
{
    std::array<display::ElementTreeNode, 1> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 5, .y = 3}, display::LayoutDirection::RightToLeft),
        [](auto&) {}));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 12, .green = 34, .blue = 56});
    const display::LogicalFramebuffer original = framebuffer;

    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

TEST(ElementTreeRenderer, SkipsNullBitmapAndUnsupportedPayloads)
{
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 1, .green = 2, .blue = 3}};
    const display::BitmapFile visible_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<display::ElementTreeNode, 5> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 0}, display::LayoutDirection::BottomToTop),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 0, .y = 0}, nullptr)));
            EXPECT_TRUE(children.add_terminal(make_text({.value = 3}, {.x = 1, .y = 0}, "skip")));
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 4}, {.x = 2, .y = 0}, {.width = 4, .height = 4})));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 5}, {.x = 3, .y = 1}, &visible_bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 40, .green = 50, .blue = 60});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 1, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 2, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 3, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 3, 1, 1, 2, 3);
}

TEST(ElementTreeRenderer, DoesNotClearUnrelatedFramebufferPixels)
{
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 200, .green = 10, .blue = 20}};
    const display::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<display::ElementTreeNode, 1> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_terminal(make_bitmap({.value = 1}, {.x = 7, .y = 1}, &bitmap)));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 10, .green = 20, .blue = 30});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 7, 1, 200, 10, 20);
    expect_rgb_at(framebuffer, 0, 0, 10, 20, 30);
    expect_rgb_at(framebuffer, 7, 0, 10, 20, 30);
    expect_rgb_at(framebuffer, 8, 1, 10, 20, 30);
    expect_rgb_at(framebuffer, 31, 7, 10, 20, 30);
}
