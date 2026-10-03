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

graphics::Element make_container(
    graphics::ElementId id,
    graphics::Position position,
    graphics::StackDirection layout_direction,
    graphics::LayoutSystem layout_system = graphics::LayoutSystem::ChildDefinedPositions,
    std::optional<graphics::Paint> paint = std::nullopt)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = graphics::ContainerElementPayload{
            .layout_direction = layout_direction,
            .layout_system = layout_system,
        },
    };
}

graphics::Element make_container(
    graphics::ElementId id,
    graphics::StackDirection layout_direction,
    graphics::LayoutSystem layout_system = graphics::LayoutSystem::ChildDefinedPositions,
    std::optional<graphics::Paint> paint = std::nullopt)
{
    return make_container(id, {}, layout_direction, layout_system, paint);
}

graphics::Element make_bitmap(graphics::ElementId id, graphics::Position position,
                             const graphics::BitmapFile* bitmap)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .payload = graphics::BitmapElementPayload{.bitmap = bitmap},
    };
}

graphics::Element make_bitmap(graphics::ElementId id, const graphics::BitmapFile* bitmap)
{
    return make_bitmap(id, {}, bitmap);
}

graphics::Element make_text(graphics::ElementId id, graphics::Position position, std::string_view text,
                           const graphics::Font* font = nullptr,
                           std::optional<graphics::Paint> paint = std::nullopt)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = graphics::TextElementPayload{.text = text, .font = font},
    };
}

graphics::Element make_text(graphics::ElementId id, std::string_view text, const graphics::Font* font = nullptr,
                           std::optional<graphics::Paint> paint = std::nullopt)
{
    return make_text(id, {}, text, font, paint);
}

graphics::Element make_rectangle(graphics::ElementId id, graphics::Position position, graphics::Size size,
                                std::optional<graphics::Paint> paint = std::nullopt)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = graphics::FilledRectangleElementPayload{.size = size},
    };
}

graphics::Element make_rectangle(graphics::ElementId id, graphics::Size size,
                                std::optional<graphics::Paint> paint = std::nullopt)
{
    return make_rectangle(id, {}, size, paint);
}

// One opaque glyph. The bitmap's color only marks the foreground; text paint supplies the drawn color.
[[nodiscard]] graphics::Font make_marker_font()
{
    static const std::array<graphics::RgbColor, 1> pixels{
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
    };
    return graphics::Font{
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

[[nodiscard]] graphics::LinearGradientPaint make_gradient()
{
    return graphics::LinearGradientPaint{
        .start = {.x = 1, .y = 2},
        .end = {.x = 5, .y = 3},
        .start_color = {.red = 255, .green = 0, .blue = 0},
        .end_color = {.red = 0, .green = 0, .blue = 255},
    };
}

}  // namespace

TEST(ElementTreeRenderer, RendersBitmapRootAtItsOwnPosition)
{
    const std::array<graphics::RgbColor, 4> pixels{
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 255, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 0, .blue = 255},
        graphics::RgbColor{.red = 255, .green = 255, .blue = 0},
    };
    const graphics::BitmapFile bitmap{
        .size = {.width = 2, .height = 2},
        .pixels = pixels,
    };

    std::array<graphics::ElementTreeNode, 1> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_terminal(make_bitmap({.value = 1}, {.x = 3, .y = 2}, &bitmap)));

    graphics::LogicalFramebuffer framebuffer;
    const graphics::ElementTreeRenderer renderer;
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
    const std::array<graphics::RgbColor, 1> pixels{graphics::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const graphics::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<graphics::ElementTreeNode, 3> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, graphics::StackDirection::LeftToRight),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, {.x = 3, .y = 2}, graphics::StackDirection::TopToBottom),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 3}, {.x = 4, .y = 1}, &bitmap)));
                }));
        }));

    graphics::LogicalFramebuffer framebuffer;
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 9, 4, 9, 8, 7);
    expect_rgb_at(framebuffer, 2, 1, 0, 0, 0);
    expect_rgb_at(framebuffer, 5, 3, 0, 0, 0);
    expect_rgb_at(framebuffer, 4, 1, 0, 0, 0);
}

TEST(ElementTreeRenderer, LaterBitmapOverwritesEarlierBitmapsInDeclarationOrder)
{
    const std::array<graphics::RgbColor, 2> earlier_pixels{
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<graphics::RgbColor, 2> nested_pixels{
        graphics::RgbColor{.red = 0, .green = 0, .blue = 255},
        graphics::RgbColor{.red = 0, .green = 255, .blue = 255},
    };
    const std::array<graphics::RgbColor, 1> later_pixels{
        graphics::RgbColor{.red = 255, .green = 255, .blue = 255},
    };
    const graphics::BitmapFile earlier_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = earlier_pixels,
    };
    const graphics::BitmapFile nested_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = nested_pixels,
    };
    const graphics::BitmapFile later_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = later_pixels,
    };

    std::array<graphics::ElementTreeNode, 5> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 1}, graphics::StackDirection::TopToBottom),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 3, .y = 2}, &earlier_bitmap)));
            EXPECT_TRUE(children.add_container(
                make_container({.value = 3}, {.x = 2, .y = 1}, graphics::StackDirection::LeftToRight),
                [&](auto& nested) {
                    EXPECT_TRUE(
                        nested.add_terminal(make_bitmap({.value = 4}, {.x = 1, .y = 1}, &nested_bitmap)));
                }));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 5}, {.x = 3, .y = 2}, &later_bitmap)));
        }));

    graphics::LogicalFramebuffer framebuffer;
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 4, 3, 255, 255, 255);
    expect_rgb_at(framebuffer, 5, 3, 0, 255, 255);
}

TEST(ElementTreeRenderer, LeavesFramebufferUnchangedForEmptyContainer)
{
    std::array<graphics::ElementTreeNode, 1> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 5, .y = 3}, graphics::StackDirection::LeftToRight),
        [](auto&) {}));

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 12, .green = 34, .blue = 56});
    const graphics::LogicalFramebuffer original = framebuffer;

    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

TEST(ElementTreeRenderer, SkipsNullBitmapAndFontlessText)
{
    const std::array<graphics::RgbColor, 1> pixels{graphics::RgbColor{.red = 1, .green = 2, .blue = 3}};
    const graphics::BitmapFile visible_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 0}, graphics::StackDirection::TopToBottom),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 0, .y = 0}, nullptr)));
            EXPECT_TRUE(children.add_terminal(make_text({.value = 3}, {.x = 1, .y = 0}, "skip")));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 4}, {.x = 3, .y = 1}, &visible_bitmap)));
        }));

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 40, .green = 50, .blue = 60});
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 1, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 3, 1, 1, 2, 3);
}

TEST(ElementTreeRenderer, DoesNotClearUnrelatedFramebufferPixels)
{
    const std::array<graphics::RgbColor, 1> pixels{graphics::RgbColor{.red = 200, .green = 10, .blue = 20}};
    const graphics::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<graphics::ElementTreeNode, 1> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_terminal(make_bitmap({.value = 1}, {.x = 7, .y = 1}, &bitmap)));

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 10, .green = 20, .blue = 30});
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 7, 1, 200, 10, 20);
    expect_rgb_at(framebuffer, 0, 0, 10, 20, 30);
    expect_rgb_at(framebuffer, 7, 0, 10, 20, 30);
    expect_rgb_at(framebuffer, 8, 1, 10, 20, 30);
    expect_rgb_at(framebuffer, 31, 7, 10, 20, 30);
}

TEST(ElementTreeRenderer, StacksLeftToRightFromContainerOriginIgnoringChildDefinedPositions)
{
    const std::array<graphics::RgbColor, 2> first_pixels{
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<graphics::RgbColor, 1> second_pixels{
        graphics::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const graphics::BitmapFile first_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = first_pixels,
    };
    const graphics::BitmapFile second_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = second_pixels,
    };

    std::array<graphics::ElementTreeNode, 3> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 2}, graphics::StackDirection::LeftToRight,
                       graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, {.x = 7, .y = 7}, &first_bitmap)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, {.x = 4, .y = 4}, &second_bitmap)));
        }));

    graphics::LogicalFramebuffer framebuffer;
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 1, 2, 255, 0, 0);
    expect_rgb_at(framebuffer, 2, 2, 0, 255, 0);
    expect_rgb_at(framebuffer, 3, 2, 0, 0, 255);
    expect_rgb_at(framebuffer, 4, 4, 0, 0, 0);
    expect_rgb_at(framebuffer, 7, 7, 0, 0, 0);
}

TEST(ElementTreeRenderer, StacksTopToBottomFromContainerOrigin)
{
    const std::array<graphics::RgbColor, 2> first_pixels{
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<graphics::RgbColor, 1> second_pixels{
        graphics::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const graphics::BitmapFile first_bitmap{
        .size = {.width = 1, .height = 2},
        .pixels = first_pixels,
    };
    const graphics::BitmapFile second_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = second_pixels,
    };

    std::array<graphics::ElementTreeNode, 3> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, graphics::StackDirection::TopToBottom,
                       graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 2}, &first_bitmap)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, &second_bitmap)));
        }));

    graphics::LogicalFramebuffer framebuffer;
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 2, 1, 255, 0, 0);
    expect_rgb_at(framebuffer, 2, 2, 0, 255, 0);
    expect_rgb_at(framebuffer, 2, 3, 0, 0, 255);
    expect_rgb_at(framebuffer, 3, 1, 0, 0, 0);
}

TEST(ElementTreeRenderer, StackedParentAdvancesPastPreviousContainerSize)
{
    const std::array<graphics::RgbColor, 2> nested_pixels{
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 255, .blue = 0},
    };
    const std::array<graphics::RgbColor, 1> following_pixels{
        graphics::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const graphics::BitmapFile nested_bitmap{
        .size = {.width = 2, .height = 1},
        .pixels = nested_pixels,
    };
    const graphics::BitmapFile following_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = following_pixels,
    };

    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 1}, graphics::StackDirection::LeftToRight,
                       graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, graphics::StackDirection::LeftToRight),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 3}, &nested_bitmap)));
                }));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 4}, &following_bitmap)));
        }));

    graphics::LogicalFramebuffer framebuffer;
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 1, 255, 0, 0);
    expect_rgb_at(framebuffer, 1, 1, 0, 255, 0);
    expect_rgb_at(framebuffer, 2, 1, 0, 0, 255);
}

TEST(ElementTreeRenderer, StackedContainerSizeUsesTheTallerChild)
{
    const std::array<graphics::RgbColor, 1> short_pixels{graphics::RgbColor{.red = 255, .green = 0, .blue = 0}};
    const std::array<graphics::RgbColor, 2> tall_pixels{
        graphics::RgbColor{.red = 0, .green = 255, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const std::array<graphics::RgbColor, 1> following_pixels{
        graphics::RgbColor{.red = 255, .green = 255, .blue = 255},
    };
    const graphics::BitmapFile short_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = short_pixels,
    };
    const graphics::BitmapFile tall_bitmap{
        .size = {.width = 1, .height = 2},
        .pixels = tall_pixels,
    };
    const graphics::BitmapFile following_bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = following_pixels,
    };

    std::array<graphics::ElementTreeNode, 5> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 0}, graphics::StackDirection::TopToBottom,
                       graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, graphics::StackDirection::LeftToRight,
                               graphics::LayoutSystem::Stacked),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 3}, &short_bitmap)));
                    EXPECT_TRUE(nested.add_terminal(make_bitmap({.value = 4}, &tall_bitmap)));
                }));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 5}, &following_bitmap)));
        }));

    graphics::LogicalFramebuffer framebuffer;
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 255, 0, 0);
    expect_rgb_at(framebuffer, 1, 0, 0, 255, 0);
    expect_rgb_at(framebuffer, 1, 1, 0, 0, 255);
    expect_rgb_at(framebuffer, 0, 1, 0, 0, 0);
    expect_rgb_at(framebuffer, 0, 2, 255, 255, 255);
}

TEST(ElementTreeRenderer, StackedLayoutAdvancesPastGradientRectangleWithoutPainting)
{
    const std::array<graphics::RgbColor, 1> pixels{graphics::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const graphics::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };
    const graphics::LinearGradientPaint gradient{
        .start = {},
        .end = {.x = 2, .y = 0},
        .start_color = {.red = 255, .green = 0, .blue = 0},
        .end_color = {.red = 0, .green = 0, .blue = 255},
    };

    std::array<graphics::ElementTreeNode, 3> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, graphics::StackDirection::LeftToRight, graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 2}, {.width = 2, .height = 1}, graphics::Paint{gradient})));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, &bitmap)));
        }));

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 40, .green = 50, .blue = 60});
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 1, 0, 40, 50, 60);
    expect_rgb_at(framebuffer, 2, 0, 9, 8, 7);
}

TEST(ElementTreeRenderer, RendersSolidRectangleRelativeToItsContainer)
{
    const graphics::SolidPaint paint{.color = {.red = 0, .green = 180, .blue = 20}};

    std::array<graphics::ElementTreeNode, 2> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, graphics::StackDirection::LeftToRight),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 2}, {.x = 3, .y = 2}, {.width = 2, .height = 2}, graphics::Paint{paint})));
        }));

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear();
    const graphics::ElementTreeRenderer renderer;
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
    const std::array<graphics::RgbColor, 1> pixels{graphics::RgbColor{.red = 1, .green = 2, .blue = 3}};
    const graphics::BitmapFile bitmap{
        .size = {.width = 1, .height = 1},
        .pixels = pixels,
    };

    std::array<graphics::ElementTreeNode, 3> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 4, .y = 2}, graphics::StackDirection::LeftToRight,
                       graphics::LayoutSystem::Stacked),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_text({.value = 2}, "hi", nullptr)));
            EXPECT_TRUE(children.add_terminal(make_bitmap({.value = 3}, &bitmap)));
        }));

    graphics::LogicalFramebuffer framebuffer;
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 4, 2, 1, 2, 3);
    expect_rgb_at(framebuffer, 5, 2, 0, 0, 0);
}

TEST(ElementTreeRenderer, RendersSolidTextRelativeToItsContainer)
{
    const std::array<graphics::RgbColor, 1> character{graphics::RgbColor{.red = 255, .green = 0, .blue = 0}};
    const graphics::Font font{
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
    const graphics::SolidPaint paint{.color = {.red = 0, .green = 180, .blue = 20}};

    std::array<graphics::ElementTreeNode, 2> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 2, .y = 1}, graphics::StackDirection::LeftToRight),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_text({.value = 2}, {.x = 3, .y = 2}, "A", &font, graphics::Paint{paint})));
        }));

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 4, .green = 5, .blue = 6});
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 5, 3, 0, 180, 20);
    expect_rgb_at(framebuffer, 2, 1, 4, 5, 6);
    expect_rgb_at(framebuffer, 4, 3, 4, 5, 6);
    expect_rgb_at(framebuffer, 6, 3, 4, 5, 6);
}

TEST(ElementTreeRenderer, InheritsSolidPaintForRectangleAndText)
{
    const graphics::Font font = make_marker_font();
    const graphics::SolidPaint inherited{.color = {.red = 0, .green = 180, .blue = 20}};

    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 1, .y = 0}, graphics::StackDirection::TopToBottom,
                       graphics::LayoutSystem::ChildDefinedPositions, graphics::Paint{inherited}),
        [&](auto& children) {
            EXPECT_TRUE(children.add_container(
                make_container({.value = 2}, {.x = 1, .y = 1}, graphics::StackDirection::LeftToRight),
                [&](auto& nested) {
                    EXPECT_TRUE(nested.add_terminal(
                        make_rectangle({.value = 3}, {.x = 1, .y = 0}, {.width = 1, .height = 1})));
                    EXPECT_TRUE(nested.add_terminal(make_text({.value = 4}, {.x = 2, .y = 0}, "A", &font)));
                }));
        }));

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 4, .green = 5, .blue = 6});
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 3, 1, 0, 180, 20);
    expect_rgb_at(framebuffer, 4, 1, 0, 180, 20);
    expect_rgb_at(framebuffer, 2, 1, 4, 5, 6);
    expect_rgb_at(framebuffer, 1, 0, 4, 5, 6);
}

TEST(ElementTreeRenderer, ExplicitPaintOverridesInheritedPaint)
{
    const graphics::Font font = make_marker_font();
    const graphics::SolidPaint inherited{.color = {.red = 10, .green = 20, .blue = 30}};
    const graphics::SolidPaint rectangle_override{.color = {.red = 200, .green = 10, .blue = 0}};
    const graphics::SolidPaint text_override{.color = {.red = 0, .green = 0, .blue = 90}};

    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTreeBuilder builder{storage};
    ASSERT_TRUE(builder.add_container(
        make_container({.value = 1}, {.x = 0, .y = 1}, graphics::StackDirection::LeftToRight,
                       graphics::LayoutSystem::ChildDefinedPositions, graphics::Paint{inherited}),
        [&](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_rectangle(
                {.value = 2}, {.x = 0, .y = 0}, {.width = 1, .height = 1}, graphics::Paint{rectangle_override})));
            EXPECT_TRUE(children.add_terminal(
                make_rectangle({.value = 3}, {.x = 2, .y = 0}, {.width = 1, .height = 1})));
            EXPECT_TRUE(children.add_terminal(
                make_text({.value = 4}, {.x = 4, .y = 0}, "A", &font, graphics::Paint{text_override})));
        }));

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 4, .green = 5, .blue = 6});
    const graphics::ElementTreeRenderer renderer;
    renderer.render(builder.get_tree(), framebuffer);

    expect_rgb_at(framebuffer, 0, 1, 200, 10, 0);
    expect_rgb_at(framebuffer, 2, 1, 10, 20, 30);
    expect_rgb_at(framebuffer, 4, 1, 0, 0, 90);
    expect_rgb_at(framebuffer, 1, 1, 4, 5, 6);
    expect_rgb_at(framebuffer, 3, 1, 4, 5, 6);
}

TEST(ElementTreeRenderer, DoesNotDrawTerminalWithNoPaintedAncestor)
{
    const graphics::Font font = make_marker_font();
    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 40, .green = 50, .blue = 60});
    const graphics::ElementTreeRenderer renderer;

    {
        std::array<graphics::ElementTreeNode, 1> storage{};
        graphics::ElementTreeBuilder builder{storage};
        ASSERT_TRUE(builder.add_terminal(
            make_rectangle({.value = 1}, {.x = 0, .y = 0}, {.width = 2, .height = 1})));
        renderer.render(builder.get_tree(), framebuffer);
    }
    {
        std::array<graphics::ElementTreeNode, 1> storage{};
        graphics::ElementTreeBuilder builder{storage};
        ASSERT_TRUE(builder.add_terminal(make_text({.value = 1}, {.x = 0, .y = 2}, "A", &font)));
        renderer.render(builder.get_tree(), framebuffer);
    }
    {
        std::array<graphics::ElementTreeNode, 3> storage{};
        graphics::ElementTreeBuilder builder{storage};
        ASSERT_TRUE(builder.add_container(
            make_container({.value = 1}, {.x = 4, .y = 0}, graphics::StackDirection::TopToBottom),
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
