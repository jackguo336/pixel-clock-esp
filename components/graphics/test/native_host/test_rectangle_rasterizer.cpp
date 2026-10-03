#include <cstdint>
#include <limits>

#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"
#include "rectangle_rasterizer.hpp"
#include "utils.hpp"

using graphics_test::expect_rgb_at;
using graphics_test::framebuffers_equal;
using graphics_test::solid_sampler;

TEST(RectangleRasterizer, FillsRectangleAtRequestedOrigin)
{
    graphics::LogicalFramebuffer framebuffer;
    const graphics::RectangleRasterizer rasterizer;
    rasterizer.rasterize(graphics::Size{.width = 2, .height = 2},
                          solid_sampler({.red = 255, .green = 0, .blue = 0}),
                          graphics::Position{.x = 3, .y = 2}, framebuffer);

    expect_rgb_at(framebuffer, 3, 2, 255, 0, 0);
    expect_rgb_at(framebuffer, 4, 2, 255, 0, 0);
    expect_rgb_at(framebuffer, 3, 3, 255, 0, 0);
    expect_rgb_at(framebuffer, 4, 3, 255, 0, 0);
    expect_rgb_at(framebuffer, 2, 2, 0, 0, 0);
    expect_rgb_at(framebuffer, 5, 2, 0, 0, 0);
    expect_rgb_at(framebuffer, 3, 1, 0, 0, 0);
    expect_rgb_at(framebuffer, 3, 4, 0, 0, 0);
}

TEST(RectangleRasterizer, WritesBlackOverExistingPixels)
{
    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 10, .green = 20, .blue = 30});
    const graphics::RectangleRasterizer rasterizer;
    rasterizer.rasterize(graphics::Size{.width = 2, .height = 1},
                          solid_sampler({.red = 0, .green = 0, .blue = 0}),
                          graphics::Position{.x = 0, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 0, 0, 0);
    expect_rgb_at(framebuffer, 1, 0, 0, 0, 0);
    expect_rgb_at(framebuffer, 2, 0, 10, 20, 30);
}

TEST(RectangleRasterizer, ClipsLeftEdgeIndependently)
{
    graphics::LogicalFramebuffer framebuffer;
    const graphics::RectangleRasterizer rasterizer;
    rasterizer.rasterize(graphics::Size{.width = 2, .height = 1},
                          solid_sampler({.red = 1, .green = 0, .blue = 0}),
                          graphics::Position{.x = -1, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 1, 0, 0);
    expect_rgb_at(framebuffer, 1, 0, 0, 0, 0);
}

TEST(RectangleRasterizer, ClipsTopEdgeIndependently)
{
    graphics::LogicalFramebuffer framebuffer;
    const graphics::RectangleRasterizer rasterizer;
    rasterizer.rasterize(graphics::Size{.width = 1, .height = 2},
                          solid_sampler({.red = 1, .green = 0, .blue = 0}),
                          graphics::Position{.x = 0, .y = -1}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 1, 0, 0);
    expect_rgb_at(framebuffer, 0, 1, 0, 0, 0);
}

TEST(RectangleRasterizer, ClipsRightEdgeIndependently)
{
    graphics::LogicalFramebuffer framebuffer;
    const graphics::RectangleRasterizer rasterizer;
    rasterizer.rasterize(graphics::Size{.width = 2, .height = 1},
                          solid_sampler({.red = 1, .green = 0, .blue = 0}),
                          graphics::Position{.x = 31, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 31, 0, 1, 0, 0);
    expect_rgb_at(framebuffer, 30, 0, 0, 0, 0);
}

TEST(RectangleRasterizer, ClipsBottomEdgeIndependently)
{
    graphics::LogicalFramebuffer framebuffer;
    const graphics::RectangleRasterizer rasterizer;
    rasterizer.rasterize(graphics::Size{.width = 1, .height = 2},
                          solid_sampler({.red = 1, .green = 0, .blue = 0}),
                          graphics::Position{.x = 0, .y = 7}, framebuffer);

    expect_rgb_at(framebuffer, 0, 7, 1, 0, 0);
    expect_rgb_at(framebuffer, 0, 6, 0, 0, 0);
}

TEST(RectangleRasterizer, LeavesFramebufferUnchangedWhenEmptyOrFullyOffCanvas)
{
    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 4, .green = 5, .blue = 6});
    const graphics::LogicalFramebuffer original = framebuffer;
    const graphics::RectangleRasterizer rasterizer;
    const graphics::ColorSampler color = solid_sampler({.red = 255, .green = 0, .blue = 0});

    rasterizer.rasterize(graphics::Size{.width = 0, .height = 4}, color, graphics::Position{.x = 0, .y = 0},
                          framebuffer);
    rasterizer.rasterize(graphics::Size{.width = 4, .height = 0}, color, graphics::Position{.x = 0, .y = 0},
                          framebuffer);
    rasterizer.rasterize(graphics::Size{}, color, graphics::Position{}, framebuffer);
    rasterizer.rasterize(graphics::Size{.width = 1, .height = 1}, color, graphics::Position{.x = 32, .y = 0},
                          framebuffer);
    rasterizer.rasterize(graphics::Size{.width = 1, .height = 1}, color, graphics::Position{.x = 0, .y = 8},
                          framebuffer);
    rasterizer.rasterize(graphics::Size{.width = 1, .height = 1}, color, graphics::Position{.x = -1, .y = 0},
                          framebuffer);
    rasterizer.rasterize(graphics::Size{.width = 1, .height = 1}, color, graphics::Position{.x = 0, .y = -1},
                          framebuffer);
    rasterizer.rasterize(graphics::Size{.width = 5, .height = 3}, color, graphics::Position{.x = -5, .y = -3},
                          framebuffer);

    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

namespace {

[[nodiscard]] graphics::ColorSampler red_blue_sampler(float angle_degrees, graphics::Position origin,
                                                       graphics::Size size)
{
    const graphics::LinearGradientPaint gradient{
        .angle_degrees = angle_degrees,
        .color_stop_count = 2,
        .color_stops =
            {
                graphics::GradientColorStop{.offset = 0.0f, .color = {.red = 255, .green = 0, .blue = 0}},
                graphics::GradientColorStop{.offset = 1.0f, .color = {.red = 0, .green = 0, .blue = 255}},
            },
    };
    return graphics::ColorSampler{graphics::Paint{gradient}, origin, size};
}

TEST(RectangleRasterizer, SamplesGradientAtEachFilledPixel)
{
    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 9, .green = 9, .blue = 9});
    const graphics::RectangleRasterizer rasterizer;
    const graphics::Position origin{.x = 1, .y = 2};
    const graphics::Size size{.width = 3, .height = 1};
    rasterizer.rasterize(size, red_blue_sampler(90.0f, origin, size), origin, framebuffer);

    expect_rgb_at(framebuffer, 1, 2, 255, 0, 0);
    expect_rgb_at(framebuffer, 2, 2, 170, 0, 85);
    expect_rgb_at(framebuffer, 3, 2, 85, 0, 170);
    expect_rgb_at(framebuffer, 0, 2, 9, 9, 9);
    expect_rgb_at(framebuffer, 4, 2, 9, 9, 9);
    expect_rgb_at(framebuffer, 1, 1, 9, 9, 9);
    expect_rgb_at(framebuffer, 1, 3, 9, 9, 9);

    graphics::LogicalFramebuffer vertical;
    vertical.clear(graphics::RgbColor{.red = 9, .green = 9, .blue = 9});
    const graphics::Position vertical_origin{.x = 2, .y = 3};
    const graphics::Size vertical_size{.width = 1, .height = 2};
    rasterizer.rasterize(vertical_size, red_blue_sampler(0.0f, vertical_origin, vertical_size), vertical_origin,
                          vertical);

    expect_rgb_at(vertical, 2, 3, 0, 0, 255);
    expect_rgb_at(vertical, 2, 4, 128, 0, 128);
    expect_rgb_at(vertical, 1, 3, 9, 9, 9);
    expect_rgb_at(vertical, 3, 3, 9, 9, 9);
    expect_rgb_at(vertical, 2, 2, 9, 9, 9);
    expect_rgb_at(vertical, 2, 5, 9, 9, 9);
}

TEST(RectangleRasterizer, GradientClipUsesCanvasPositionInsideThePaintBounds)
{
    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 4, .green = 5, .blue = 6});
    const graphics::RectangleRasterizer rasterizer;
    const graphics::Position origin{.x = -1, .y = 0};
    const graphics::Size size{.width = 3, .height = 1};
    rasterizer.rasterize(size, red_blue_sampler(90.0f, origin, size), origin, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 170, 0, 85);
    expect_rgb_at(framebuffer, 1, 0, 85, 0, 170);
    expect_rgb_at(framebuffer, 2, 0, 4, 5, 6);
}

}  // namespace