#include <cstdint>
#include <limits>

#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"
#include "rectangle_rasterizer.hpp"
#include "utils.hpp"

using graphics_test::expect_rgb_at;
using graphics_test::framebuffers_equal;

TEST(RectangleRasterizer, FillsRectangleAtRequestedOrigin)
{
    display::LogicalFramebuffer framebuffer;
    const display::RectangleRasterizer rasterizer;
    rasterizer.rasterize(display::Size{.width = 2, .height = 2},
                          display::RgbColor{.red = 255, .green = 0, .blue = 0},
                          display::Position{.x = 3, .y = 2}, framebuffer);

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
    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 10, .green = 20, .blue = 30});
    const display::RectangleRasterizer rasterizer;
    rasterizer.rasterize(display::Size{.width = 2, .height = 1},
                          display::RgbColor{.red = 0, .green = 0, .blue = 0},
                          display::Position{.x = 0, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 0, 0, 0);
    expect_rgb_at(framebuffer, 1, 0, 0, 0, 0);
    expect_rgb_at(framebuffer, 2, 0, 10, 20, 30);
}

TEST(RectangleRasterizer, ClipsLeftEdgeIndependently)
{
    display::LogicalFramebuffer framebuffer;
    const display::RectangleRasterizer rasterizer;
    rasterizer.rasterize(display::Size{.width = 2, .height = 1},
                          display::RgbColor{.red = 1, .green = 0, .blue = 0},
                          display::Position{.x = -1, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 1, 0, 0);
    expect_rgb_at(framebuffer, 1, 0, 0, 0, 0);
}

TEST(RectangleRasterizer, ClipsTopEdgeIndependently)
{
    display::LogicalFramebuffer framebuffer;
    const display::RectangleRasterizer rasterizer;
    rasterizer.rasterize(display::Size{.width = 1, .height = 2},
                          display::RgbColor{.red = 1, .green = 0, .blue = 0},
                          display::Position{.x = 0, .y = -1}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 1, 0, 0);
    expect_rgb_at(framebuffer, 0, 1, 0, 0, 0);
}

TEST(RectangleRasterizer, ClipsRightEdgeIndependently)
{
    display::LogicalFramebuffer framebuffer;
    const display::RectangleRasterizer rasterizer;
    rasterizer.rasterize(display::Size{.width = 2, .height = 1},
                          display::RgbColor{.red = 1, .green = 0, .blue = 0},
                          display::Position{.x = 31, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 31, 0, 1, 0, 0);
    expect_rgb_at(framebuffer, 30, 0, 0, 0, 0);
}

TEST(RectangleRasterizer, ClipsBottomEdgeIndependently)
{
    display::LogicalFramebuffer framebuffer;
    const display::RectangleRasterizer rasterizer;
    rasterizer.rasterize(display::Size{.width = 1, .height = 2},
                          display::RgbColor{.red = 1, .green = 0, .blue = 0},
                          display::Position{.x = 0, .y = 7}, framebuffer);

    expect_rgb_at(framebuffer, 0, 7, 1, 0, 0);
    expect_rgb_at(framebuffer, 0, 6, 0, 0, 0);
}

TEST(RectangleRasterizer, LeavesFramebufferUnchangedWhenEmptyOrFullyOffCanvas)
{
    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 4, .green = 5, .blue = 6});
    const display::LogicalFramebuffer original = framebuffer;
    const display::RectangleRasterizer rasterizer;
    const display::RgbColor color{.red = 255, .green = 0, .blue = 0};

    rasterizer.rasterize(display::Size{.width = 0, .height = 4}, color, display::Position{.x = 0, .y = 0},
                          framebuffer);
    rasterizer.rasterize(display::Size{.width = 4, .height = 0}, color, display::Position{.x = 0, .y = 0},
                          framebuffer);
    rasterizer.rasterize(display::Size{}, color, display::Position{}, framebuffer);
    rasterizer.rasterize(display::Size{.width = 1, .height = 1}, color, display::Position{.x = 32, .y = 0},
                          framebuffer);
    rasterizer.rasterize(display::Size{.width = 1, .height = 1}, color, display::Position{.x = 0, .y = 8},
                          framebuffer);
    rasterizer.rasterize(display::Size{.width = 1, .height = 1}, color, display::Position{.x = -1, .y = 0},
                          framebuffer);
    rasterizer.rasterize(display::Size{.width = 1, .height = 1}, color, display::Position{.x = 0, .y = -1},
                          framebuffer);
    rasterizer.rasterize(display::Size{.width = 5, .height = 3}, color, display::Position{.x = -5, .y = -3},
                          framebuffer);

    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}