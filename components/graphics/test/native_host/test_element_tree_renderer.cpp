#include <array>
#include <optional>
#include <string_view>

#include "bitmap_file.hpp"
#include "element_tree.hpp"
#include "elements.hpp"
#include "element_tree_renderer.hpp"
#include "font.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"
#include "utils.hpp"

using graphics_test::expect_rgb_at;
using graphics_test::framebuffers_equal;

namespace {

display::Element make_container(
    display::ElementId id,
    display::Position position,
    display::StackDirection layout_direction,
    display::LayoutSystem layout_system = display::LayoutSystem::ChildDefinedPositions,
    std::optional<display::Paint> paint = std::nullopt)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = display::ContainerElementPayload{
            .layout_direction = layout_direction,
            .layout_system = layout_system,
        },
    };
}

display::Element make_container(
    display::ElementId id,
    display::StackDirection layout_direction,
    display::LayoutSystem layout_system = display::LayoutSystem::ChildDefinedPositions,
    std::optional<display::Paint> paint = std::nullopt)
{
    return make_container(id, {}, layout_direction, layout_system, paint);
}

display::Element make_bitmap(display::ElementId id, display::Position position,
                             const display::BitmapFile* bitmap)
{
    return display::Element{
        .id = id,
        .position = position,
        .payload = display::BitmapElementPayload{.bitmap = bitmap},
    };
}

display::Element make_bitmap(display::ElementId id, const display::BitmapFile* bitmap)
{
    return make_bitmap(id, {}, bitmap);
}

display::Element make_text(display::ElementId id, display::Position position, std::string_view text,
                           const display::Font* font = nullptr,
                           std::optional<display::Paint> paint = std::nullopt)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = display::TextElementPayload{.text = text, .font = font},
    };
}

display::Element make_text(display::ElementId id, std::string_view text, const display::Font* font = nullptr,
                           std::optional<display::Paint> paint = std::nullopt)
{
    return make_text(id, {}, text, font, paint);
}

display::Element make_rectangle(display::ElementId id, display::Position position, display::Size size,
                                std::optional<display::Paint> paint = std::nullopt)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = display::FilledRectangleElementPayload{.size = size},
    };
}

display::Element make_rectangle(display::ElementId id, display::Size size,
                                std::optional<display::Paint> paint = std::nullopt)
{
    return make_rectangle(id, {}, size, paint);
}

// One opaque glyph. The bitmap's color only marks the foreground; text paint supplies the drawn color.
[[nodiscard]] display::Font make_marker_font()
{
    static const std::array<display::RgbColor, 1> pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
    };
    return display::Font{
        .config = {
            .character_size = {.width = 1, .height = 1},
            .character_lookup = "A",
            .bitmap_path = "/assets/fonts/bitmap.bmp",
        },
        .bitmap = {
            .size = {.width = 1, .height = 1},
            .pixels = pixels,
        },
    };
}

[[nodiscard]] display::LinearGradientPaint make_gradient()
{
    return display::LinearGradientPaint{
        .start = {.x = 1, .y = 2},
        .end = {.x = 5, .y = 3},
        .start_color = {.red = 255, .green = 0, .blue = 0},
        .end_color = {.red = 0, .green = 0, .blue = 255},
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

TEST(ElementTreeRenderer, ChildElementPositionsAreRelativeToTheirContainerOrigin)
{
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<display::ElementTreeNode, 3> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, display::StackDirection::LeftToRight),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, {.x = 3, .y = 2}, display::StackDirection::TopToBottom),
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

TEST(ElementTreeRenderer, LaterBitmapOverwritesEarlierBitmapsInDeclarationOrder)
{
    const std::array<display::RgbColor, 2> earlier_pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<display::RgbColor, 2> nested_pixels{
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
        display::RgbColor{.red = 0, .green = 255, .blue = 255},
    };
    const std::array<display::RgbColor, 1> later_pixels{
        display::RgbColor{.red = 255, .green = 255, .blue = 255},
    };
    const display::BitmapFile earlier_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = earlier_pixels,
    };
    const display::BitmapFile nested_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = nested_pixels,
    };
    const display::BitmapFile later_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = later_pixels,
    };

    std::array<display::ElementTreeNode, 5> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 1}, display::StackDirection::TopToBottom),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 3, .y = 2}, &earlier_bitmap)));
            EXPECT_TRUE(children.add_container(
                make_container({.value = 3}, {.x = 2, .y = 1}, display::StackDirection::LeftToRight),
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
    expect_rgb_at(framebuffer, 5, 3, 0, 255, 255);
}

TEST(ElementTreeRenderer, LeavesFramebufferUnchangedForEmptyContainer)
{
    std::array<display::ElementTreeNode, 1> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 5, .y = 3}, display::StackDirection::LeftToRight),
        [](auto&) {}));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 12, .green = 34, .blue = 56});
    const display::LogicalFramebuffer original = framebuffer;

    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

TEST(ElementTreeRenderer, SkipsNullBitmapAndFontlessText)
{
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 1, .green = 2, .blue = 3}};
    const display::BitmapFile visible_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 0}, display::StackDirection::TopToBottom),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 0, .y = 0}, nullptr)));
            EXPECT_TRUE(children.add_terminal(make_text({.value = 3}, {.x = 1, .y = 0}, "skip")));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 4}, {.x = 3, .y = 1}, &visible_bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 40, .green = 50, .blue = 60});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 1, 0, 40, 50, 60);
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

TEST(ElementTreeRenderer, StacksLeftToRightFromContainerOriginIgnoringChildDefinedPositions)
{
    const std::array<display::RgbColor, 2> first_pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<display::RgbColor, 1> second_pixels{
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const display::BitmapFile first_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = first_pixels,
    };
    const display::BitmapFile second_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = second_pixels,
    };

    std::array<display::ElementTreeNode, 3> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 2}, display::StackDirection::LeftToRight,
                       display::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 7, .y = 7}, &first_bitmap)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, {.x = 4, .y = 4}, &second_bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 1, 2, 255, 0, 0);
    expect_rgb_at(framebuffer, 2, 2, 0, 255, 0);
    expect_rgb_at(framebuffer, 3, 2, 0, 0, 255);
    expect_rgb_at(framebuffer, 4, 4, 0, 0, 0);
    expect_rgb_at(framebuffer, 7, 7, 0, 0, 0);
}

TEST(ElementTreeRenderer, StacksTopToBottomFromContainerOrigin)
{
    const std::array<display::RgbColor, 2> first_pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<display::RgbColor, 1> second_pixels{
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const display::BitmapFile first_bitmap{
        .size = {.width = 1, .height = 2},
        .pixels = first_pixels,
    };
    const display::BitmapFile second_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = second_pixels,
    };

    std::array<display::ElementTreeNode, 3> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, display::StackDirection::TopToBottom,
                       display::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, &first_bitmap)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, &second_bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 2, 1, 255, 0, 0);
    expect_rgb_at(framebuffer, 2, 2, 0, 255, 0);
    expect_rgb_at(framebuffer, 2, 3, 0, 0, 255);
    expect_rgb_at(framebuffer, 3, 1, 0, 0, 0);
}

TEST(ElementTreeRenderer, StackedParentAdvancesPastPreviousContainerSize)
{
    const std::array<display::RgbColor, 2> nested_pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<display::RgbColor, 1> following_pixels{
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const display::BitmapFile nested_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = nested_pixels,
    };
    const display::BitmapFile following_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = following_pixels,
    };

    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 1}, display::StackDirection::LeftToRight,
                       display::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, display::StackDirection::LeftToRight),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 3}, &nested_bitmap)));
                }));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 4}, &following_bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 1, 255, 0, 0);
    expect_rgb_at(framebuffer, 1, 1, 0, 255, 0);
    expect_rgb_at(framebuffer, 2, 1, 0, 0, 255);
}

TEST(ElementTreeRenderer, StackedContainerSizeUsesTheTallerChild)
{
    const std::array<display::RgbColor, 1> short_pixels{display::RgbColor{.red = 255, .green = 0, .blue = 0}};
    const std::array<display::RgbColor, 2> tall_pixels{
        display::RgbColor{.red = 0, .green = 255, .blue = 0},
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const std::array<display::RgbColor, 1> following_pixels{
        display::RgbColor{.red = 255, .green = 255, .blue = 255},
    };
    const display::BitmapFile short_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = short_pixels,
    };
    const display::BitmapFile tall_bitmap{
        .size = {.width = 1, .height = 2},
        .pixels = tall_pixels,
    };
    const display::BitmapFile following_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = following_pixels,
    };

    std::array<display::ElementTreeNode, 5> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 0}, display::StackDirection::TopToBottom,
                       display::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, display::StackDirection::LeftToRight,
                               display::LayoutSystem::Stacked),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 3}, &short_bitmap)));
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 4}, &tall_bitmap)));
                }));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 5}, &following_bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 255, 0, 0);
    expect_rgb_at(framebuffer, 1, 0, 0, 255, 0);
    expect_rgb_at(framebuffer, 1, 1, 0, 0, 255);
    expect_rgb_at(framebuffer, 0, 1, 0, 0, 0);
    expect_rgb_at(framebuffer, 0, 2, 255, 255, 255);
}

TEST(ElementTreeRenderer, StackedLayoutAdvancesPastGradientRectangleWithoutPainting)
{
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };
    const display::LinearGradientPaint gradient{
        .start = {},
        .end = {.x = 2, .y = 0},
        .start_color = {.red = 255, .green = 0, .blue = 0},
        .end_color = {.red = 0, .green = 0, .blue = 255},
    };

    std::array<display::ElementTreeNode, 3> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, display::StackDirection::LeftToRight, display::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 2}, {.width = 2, .height = 1}, display::Paint{gradient})));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, &bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 40, .green = 50, .blue = 60});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 1, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 2, 0, 9, 8, 7);
}

TEST(ElementTreeRenderer, RendersSolidRectangleRelativeToItsContainer)
{
    const display::SolidPaint paint{.color = {.red = 0, .green = 180, .blue = 20}};

    std::array<display::ElementTreeNode, 2> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, display::StackDirection::LeftToRight),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 2}, {.x = 3, .y = 2}, {.width = 2, .height = 2}, display::Paint{paint})));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear();
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 5, 3, 0, 180, 20);
    expect_rgb_at(framebuffer, 6, 3, 0, 180, 20);
    expect_rgb_at(framebuffer, 5, 4, 0, 180, 20);
    expect_rgb_at(framebuffer, 6, 4, 0, 180, 20);
    expect_rgb_at(framebuffer, 4, 3, 0, 0, 0);
    expect_rgb_at(framebuffer, 7, 3, 0, 0, 0);
    expect_rgb_at(framebuffer, 5, 2, 0, 0, 0);
    expect_rgb_at(framebuffer, 5, 5, 0, 0, 0);
    expect_rgb_at(framebuffer, 2, 1, 0, 0, 0);
}

TEST(ElementTreeRenderer, StackedLayoutGivesNullFontTextNoSize)
{
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 1, .green = 2, .blue = 3}};
    const display::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<display::ElementTreeNode, 3> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 4, .y = 2}, display::StackDirection::LeftToRight,
                       display::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_text({.value = 2}, "hi", nullptr)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, &bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 4, 2, 1, 2, 3);
    expect_rgb_at(framebuffer, 5, 2, 0, 0, 0);
}

TEST(ElementTreeRenderer, RendersSolidTextRelativeToItsContainer)
{
    const std::array<display::RgbColor, 1> character{display::RgbColor{.red = 255, .green = 0, .blue = 0}};
    const display::Font font{
        .config = {
            .character_size = {.width = 1, .height = 1},
            .character_lookup = "A",
            .bitmap_path = "/assets/fonts/bitmap.bmp",
        },
        .bitmap = {
            .size = {.width = 1, .height = 1},
            .pixels = character,
        },
    };
    const display::SolidPaint paint{.color = {.red = 0, .green = 180, .blue = 20}};

    std::array<display::ElementTreeNode, 2> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, display::StackDirection::LeftToRight),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_text({.value = 2}, {.x = 3, .y = 2}, "A", &font, display::Paint{paint})));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 4, .green = 5, .blue = 6});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 5, 3, 0, 180, 20);
    expect_rgb_at(framebuffer, 2, 1, 4, 5, 6);
    expect_rgb_at(framebuffer, 4, 3, 4, 5, 6);
    expect_rgb_at(framebuffer, 6, 3, 4, 5, 6);
}

TEST(ElementTreeRenderer, InheritsSolidPaintForRectangleAndText)
{
    const display::Font font = make_marker_font();
    const std::array<display::RgbColor, 1> bitmap_pixels{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = bitmap_pixels,
    };
    const display::SolidPaint inherited{.color = {.red = 0, .green = 180, .blue = 20}};

    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 1}, display::StackDirection::LeftToRight,
                       display::LayoutSystem::ChildDefinedPositions, display::Paint{inherited}),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 2}, {.x = 0, .y = 0}, {.width = 2, .height = 1})));
            EXPECT_TRUE(children.add_terminal(make_text({.value = 3}, {.x = 3, .y = 0}, "A", &font)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 4}, {.x = 5, .y = 0}, &bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 4, .green = 5, .blue = 6});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 1, 1, 0, 180, 20);
    expect_rgb_at(framebuffer, 2, 1, 0, 180, 20);
    expect_rgb_at(framebuffer, 4, 1, 0, 180, 20);
    expect_rgb_at(framebuffer, 6, 1, 9, 8, 7);
    expect_rgb_at(framebuffer, 3, 1, 4, 5, 6);
    expect_rgb_at(framebuffer, 5, 1, 4, 5, 6);
    expect_rgb_at(framebuffer, 1, 2, 4, 5, 6);
}

TEST(ElementTreeRenderer, InheritsSolidPaintAcrossColorlessContainer)
{
    const display::Font font = make_marker_font();
    const display::SolidPaint inherited{.color = {.red = 0, .green = 180, .blue = 20}};

    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 0}, display::StackDirection::TopToBottom,
                       display::LayoutSystem::ChildDefinedPositions, display::Paint{inherited}),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, {.x = 1, .y = 1}, display::StackDirection::LeftToRight),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(
                        make_rectangle({.value = 3}, {.x = 1, .y = 0}, {.width = 1, .height = 1})));
                    EXPECT_TRUE(nested.add_terminal(make_text({.value = 4}, {.x = 2, .y = 0}, "A", &font)));
                }));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 4, .green = 5, .blue = 6});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 3, 1, 0, 180, 20);
    expect_rgb_at(framebuffer, 4, 1, 0, 180, 20);
    expect_rgb_at(framebuffer, 2, 1, 4, 5, 6);
    expect_rgb_at(framebuffer, 1, 0, 4, 5, 6);
}

TEST(ElementTreeRenderer, ExplicitPaintOverridesInheritedPaint)
{
    const display::Font font = make_marker_font();
    const display::SolidPaint inherited{.color = {.red = 10, .green = 20, .blue = 30}};
    const display::SolidPaint rectangle_override{.color = {.red = 200, .green = 10, .blue = 0}};
    const display::SolidPaint text_override{.color = {.red = 0, .green = 0, .blue = 90}};

    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 1}, display::StackDirection::LeftToRight,
                       display::LayoutSystem::ChildDefinedPositions, display::Paint{inherited}),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_rectangle(
                {.value = 2}, {.x = 0, .y = 0}, {.width = 1, .height = 1}, display::Paint{rectangle_override})));
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 3}, {.x = 2, .y = 0}, {.width = 1, .height = 1})));
            EXPECT_TRUE(children.add_terminal(
                make_text({.value = 4}, {.x = 4, .y = 0}, "A", &font, display::Paint{text_override})));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 4, .green = 5, .blue = 6});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 1, 200, 10, 0);
    expect_rgb_at(framebuffer, 2, 1, 10, 20, 30);
    expect_rgb_at(framebuffer, 4, 1, 0, 0, 90);
    expect_rgb_at(framebuffer, 1, 1, 4, 5, 6);
    expect_rgb_at(framebuffer, 3, 1, 4, 5, 6);
}

TEST(ElementTreeRenderer, DoesNotDrawTerminalWithNoPaintedAncestor)
{
    const display::Font font = make_marker_font();
    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 40, .green = 50, .blue = 60});
    const display::ElementTreeRenderer renderer;

    {
        std::array<display::ElementTreeNode, 1> storage{};
        display::ElementTreeBuilder builder{storage};
        ASSERT_TRUE(builder.add_terminal(
            make_rectangle({.value = 1}, {.x = 0, .y = 0}, {.width = 2, .height = 1})));
        renderer.render(builder.get_tree(), framebuffer);
    }
    {
        std::array<display::ElementTreeNode, 1> storage{};
        display::ElementTreeBuilder builder{storage};
        ASSERT_TRUE(builder.add_terminal(make_text({.value = 1}, {.x = 0, .y = 2}, "A", &font)));
        renderer.render(builder.get_tree(), framebuffer);
    }
    {
        std::array<display::ElementTreeNode, 3> storage{};
        display::ElementTreeBuilder builder{storage};
        ASSERT_TRUE(builder.add_container(
            make_container({.value = 1}, {.x = 4, .y = 0}, display::StackDirection::TopToBottom),
            [&](auto& children) {
                EXPECT_TRUE(children.add_terminal(
                    make_rectangle({.value = 2}, {.x = 0, .y = 0}, {.width = 1, .height = 1})));
                EXPECT_TRUE(children.add_terminal(make_text({.value = 3}, {.x = 0, .y = 2}, "A", &font)));
            }));
        renderer.render(builder.get_tree(), framebuffer);
    }

    expect_rgb_at(framebuffer, 0, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 1, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 0, 2, 40, 50, 60);
    expect_rgb_at(framebuffer, 4, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 4, 2, 40, 50, 60);
}

TEST(ElementTreeRenderer, InheritedGradientIsNotDrawnAndRetainsRectangleLayoutSize)
{
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };
    const display::LinearGradientPaint gradient = make_gradient();

    std::array<display::ElementTreeNode, 3> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 2}, display::StackDirection::LeftToRight,
                       display::LayoutSystem::Stacked, display::Paint{gradient}),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_rectangle({.value = 2}, {.width = 2, .height = 1})));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, &bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 40, .green = 50, .blue = 60});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 1, 2, 40, 50, 60);
    expect_rgb_at(framebuffer, 2, 2, 40, 50, 60);
    expect_rgb_at(framebuffer, 3, 2, 9, 8, 7);
    expect_rgb_at(framebuffer, 4, 2, 40, 50, 60);
}

TEST(ElementTreeRenderer, InheritedGradientTextIsNotDrawnAndHasNoLayoutSize)
{
    const display::Font font = make_marker_font();
    const std::array<display::RgbColor, 1> pixels{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };
    const display::LinearGradientPaint gradient = make_gradient();

    std::array<display::ElementTreeNode, 3> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 4}, display::StackDirection::LeftToRight,
                       display::LayoutSystem::Stacked, display::Paint{gradient}),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_text({.value = 2}, "A", &font)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, &bitmap)));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 40, .green = 50, .blue = 60});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 4, 9, 8, 7);
    expect_rgb_at(framebuffer, 1, 4, 40, 50, 60);
}

TEST(ElementTreeRenderer, ExplicitSolidPaintOverridesInheritedGradient)
{
    const display::Font font = make_marker_font();
    const display::LinearGradientPaint gradient = make_gradient();
    const display::SolidPaint rectangle_paint{.color = {.red = 0, .green = 180, .blue = 20}};
    const display::SolidPaint text_paint{.color = {.red = 7, .green = 8, .blue = 9}};

    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, display::StackDirection::LeftToRight,
                       display::LayoutSystem::ChildDefinedPositions, display::Paint{gradient}),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_rectangle(
                {.value = 2}, {.x = 1, .y = 1}, {.width = 1, .height = 1}, display::Paint{rectangle_paint})));
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 3}, {.x = 3, .y = 1}, {.width = 1, .height = 1})));
            EXPECT_TRUE(children.add_terminal(
                make_text({.value = 4}, {.x = 5, .y = 1}, "A", &font, display::Paint{text_paint})));
        }));

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 40, .green = 50, .blue = 60});
    const display::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 1, 1, 0, 180, 20);
    expect_rgb_at(framebuffer, 3, 1, 40, 50, 60);
    expect_rgb_at(framebuffer, 5, 1, 7, 8, 9);
    expect_rgb_at(framebuffer, 0, 0, 40, 50, 60);
}
