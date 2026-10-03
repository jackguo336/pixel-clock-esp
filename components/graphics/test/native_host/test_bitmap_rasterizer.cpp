#include <array>

#include "bitmap_rasterizer.hpp"
#include "bitmap_file.hpp"
#include "gtest/gtest.h"
#include "logical_framebuffer.hpp"
#include "utils.hpp"

using graphics_test::expect_rgb_at;
using graphics_test::framebuffers_equal;

TEST(BitmapRasterizer, RendersMultiColorBitmapAtRequestedOrigin)
{
    const std::array<graphics::RgbColor, 4> pixels{
        graphics::RgbColor{.red = 255, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 255, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 0, .blue = 255},
        graphics::RgbColor{.red = 255, .green = 255, .blue = 0},
    };
    const graphics::BitmapFile view{
        .size = {.width = 2, .height = 2},
        .pixels = pixels,
    };

    graphics::LogicalFramebuffer framebuffer;
    const graphics::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, graphics::Position{.x = 3, .y = 2}, framebuffer);

    expect_rgb_at(framebuffer, 3, 2, 255, 0, 0);
    expect_rgb_at(framebuffer, 4, 2, 0, 255, 0);
    expect_rgb_at(framebuffer, 3, 3, 0, 0, 255);
    expect_rgb_at(framebuffer, 4, 3, 255, 255, 0);
    expect_rgb_at(framebuffer, 2, 2, 0, 0, 0);
    expect_rgb_at(framebuffer, 5, 2, 0, 0, 0);
    expect_rgb_at(framebuffer, 3, 1, 0, 0, 0);
    expect_rgb_at(framebuffer, 3, 4, 0, 0, 0);
}

TEST(BitmapRasterizer, WritesBlackSourcePixels)
{
    const std::array<graphics::RgbColor, 2> pixels{
        graphics::RgbColor{.red = 0, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 255, .green = 255, .blue = 255},
    };
    const graphics::BitmapFile view{
        .size = {.width = 2, .height = 1},
        .pixels = pixels,
    };

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 10, .green = 20, .blue = 30});
    const graphics::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, graphics::Position{.x = 0, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 0, 0, 0);
    expect_rgb_at(framebuffer, 1, 0, 255, 255, 255);
    expect_rgb_at(framebuffer, 2, 0, 10, 20, 30);
}

TEST(BitmapRasterizer, ClipsLeftEdgeIndependently)
{
    const std::array<graphics::RgbColor, 2> pixels{
        graphics::RgbColor{.red = 1, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 1, .blue = 0},
    };
    const graphics::BitmapFile view{
        .size = {.width = 2, .height = 1},
        .pixels = pixels,
    };

    graphics::LogicalFramebuffer framebuffer;
    const graphics::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, graphics::Position{.x = -1, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 0, 1, 0);
    expect_rgb_at(framebuffer, 1, 0, 0, 0, 0);
}

TEST(BitmapRasterizer, ClipsTopEdgeIndependently)
{
    const std::array<graphics::RgbColor, 2> pixels{
        graphics::RgbColor{.red = 1, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 1, .blue = 0},
    };
    const graphics::BitmapFile view{
        .size = {.width = 1, .height = 2},
        .pixels = pixels,
    };

    graphics::LogicalFramebuffer framebuffer;
    const graphics::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, graphics::Position{.x = 0, .y = -1}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 0, 1, 0);
    expect_rgb_at(framebuffer, 0, 1, 0, 0, 0);
}

TEST(BitmapRasterizer, ClipsRightEdgeIndependently)
{
    const std::array<graphics::RgbColor, 2> pixels{
        graphics::RgbColor{.red = 1, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 1, .blue = 0},
    };
    const graphics::BitmapFile view{
        .size = {.width = 2, .height = 1},
        .pixels = pixels,
    };

    graphics::LogicalFramebuffer framebuffer;
    const graphics::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, graphics::Position{.x = 31, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 31, 0, 1, 0, 0);
    expect_rgb_at(framebuffer, 30, 0, 0, 0, 0);
}

TEST(BitmapRasterizer, ClipsBottomEdgeIndependently)
{
    const std::array<graphics::RgbColor, 2> pixels{
        graphics::RgbColor{.red = 1, .green = 0, .blue = 0},
        graphics::RgbColor{.red = 0, .green = 1, .blue = 0},
    };
    const graphics::BitmapFile view{
        .size = {.width = 1, .height = 2},
        .pixels = pixels,
    };

    graphics::LogicalFramebuffer framebuffer;
    const graphics::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, graphics::Position{.x = 0, .y = 7}, framebuffer);

    expect_rgb_at(framebuffer, 0, 7, 1, 0, 0);
    expect_rgb_at(framebuffer, 0, 6, 0, 0, 0);
}

TEST(BitmapRasterizer, LeavesFramebufferUnchangedWhenFullyOffCanvasOrInvalid)
{
    const std::array<graphics::RgbColor, 1> pixel{graphics::RgbColor{.red = 255, .green = 0, .blue = 0}};
    const graphics::BitmapFile onscreen{
        .size = {.width = 1, .height = 1},
        .pixels = pixel,
    };
    const graphics::BitmapFile invalid{
        .size = {.width = 0, .height = 1},
        .pixels = pixel,
    };
    const graphics::BitmapFile undersized{
        .size = {.width = 2, .height = 1},
        .pixels = pixel,
    };

    graphics::LogicalFramebuffer framebuffer;
    framebuffer.clear(graphics::RgbColor{.red = 4, .green = 5, .blue = 6});
    const graphics::LogicalFramebuffer original = framebuffer;
    const graphics::BitmapRasterizer rasterizer;

    rasterizer.rasterize(onscreen, graphics::Position{.x = 32, .y = 0}, framebuffer);
    rasterizer.rasterize(onscreen, graphics::Position{.x = 0, .y = 8}, framebuffer);
    rasterizer.rasterize(onscreen, graphics::Position{.x = -1, .y = 0}, framebuffer);
    rasterizer.rasterize(onscreen, graphics::Position{.x = 0, .y = -1}, framebuffer);
    rasterizer.rasterize(invalid, graphics::Position{.x = 0, .y = 0}, framebuffer);
    rasterizer.rasterize(undersized, graphics::Position{.x = 0, .y = 0}, framebuffer);

    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

TEST(BitmapRasterizer, RendersDifferentViewsThroughTheSameInstance)
{
    const std::array<graphics::RgbColor, 1> first_pixels{graphics::RgbColor{.red = 9, .green = 0, .blue = 0}};
    const std::array<graphics::RgbColor, 1> second_pixels{graphics::RgbColor{.red = 0, .green = 9, .blue = 0}};
    const graphics::BitmapFile first_view{
        .size = {.width = 1, .height = 1},
        .pixels = first_pixels,
    };
    const graphics::BitmapFile second_view{
        .size = {.width = 1, .height = 1},
        .pixels = second_pixels,
    };

    graphics::LogicalFramebuffer framebuffer;
    const graphics::BitmapRasterizer rasterizer;
    rasterizer.rasterize(first_view, graphics::Position{.x = 1, .y = 1}, framebuffer);
    expect_rgb_at(framebuffer, 1, 1, 9, 0, 0);

    framebuffer.clear();
    rasterizer.rasterize(second_view, graphics::Position{.x = 2, .y = 2}, framebuffer);
    expect_rgb_at(framebuffer, 2, 2, 0, 9, 0);
    expect_rgb_at(framebuffer, 1, 1, 0, 0, 0);
}
