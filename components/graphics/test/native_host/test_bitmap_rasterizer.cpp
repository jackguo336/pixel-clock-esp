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
    const std::array<display::RgbColor, 4> pixels{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 255, .blue = 0},
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
        display::RgbColor{.red = 255, .green = 255, .blue = 0},
    };
    const display::BitmapFile view{
        .size = {.width = 2, .height = 2},
        .pixels = pixels,
    };

    display::LogicalFramebuffer framebuffer;
    const display::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, display::Position{.x = 3, .y = 2}, framebuffer);

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
    const std::array<display::RgbColor, 2> pixels{
        display::RgbColor{.red = 0, .green = 0, .blue = 0},
        display::RgbColor{.red = 255, .green = 255, .blue = 255},
    };
    const display::BitmapFile view{
        .size = {.width = 2, .height = 1},
        .pixels = pixels,
    };

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 10, .green = 20, .blue = 30});
    const display::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, display::Position{.x = 0, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 0, 0, 0);
    expect_rgb_at(framebuffer, 1, 0, 255, 255, 255);
    expect_rgb_at(framebuffer, 2, 0, 10, 20, 30);
}

TEST(BitmapRasterizer, ClipsLeftEdgeIndependently)
{
    const std::array<display::RgbColor, 2> pixels{
        display::RgbColor{.red = 1, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 1, .blue = 0},
    };
    const display::BitmapFile view{
        .size = {.width = 2, .height = 1},
        .pixels = pixels,
    };

    display::LogicalFramebuffer framebuffer;
    const display::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, display::Position{.x = -1, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 0, 1, 0);
    expect_rgb_at(framebuffer, 1, 0, 0, 0, 0);
}

TEST(BitmapRasterizer, ClipsTopEdgeIndependently)
{
    const std::array<display::RgbColor, 2> pixels{
        display::RgbColor{.red = 1, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 1, .blue = 0},
    };
    const display::BitmapFile view{
        .size = {.width = 1, .height = 2},
        .pixels = pixels,
    };

    display::LogicalFramebuffer framebuffer;
    const display::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, display::Position{.x = 0, .y = -1}, framebuffer);

    expect_rgb_at(framebuffer, 0, 0, 0, 1, 0);
    expect_rgb_at(framebuffer, 0, 1, 0, 0, 0);
}

TEST(BitmapRasterizer, ClipsRightEdgeIndependently)
{
    const std::array<display::RgbColor, 2> pixels{
        display::RgbColor{.red = 1, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 1, .blue = 0},
    };
    const display::BitmapFile view{
        .size = {.width = 2, .height = 1},
        .pixels = pixels,
    };

    display::LogicalFramebuffer framebuffer;
    const display::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, display::Position{.x = 31, .y = 0}, framebuffer);

    expect_rgb_at(framebuffer, 31, 0, 1, 0, 0);
    expect_rgb_at(framebuffer, 30, 0, 0, 0, 0);
}

TEST(BitmapRasterizer, ClipsBottomEdgeIndependently)
{
    const std::array<display::RgbColor, 2> pixels{
        display::RgbColor{.red = 1, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 1, .blue = 0},
    };
    const display::BitmapFile view{
        .size = {.width = 1, .height = 2},
        .pixels = pixels,
    };

    display::LogicalFramebuffer framebuffer;
    const display::BitmapRasterizer rasterizer;
    rasterizer.rasterize(view, display::Position{.x = 0, .y = 7}, framebuffer);

    expect_rgb_at(framebuffer, 0, 7, 1, 0, 0);
    expect_rgb_at(framebuffer, 0, 6, 0, 0, 0);
}

TEST(BitmapRasterizer, LeavesFramebufferUnchangedWhenFullyOffCanvasOrInvalid)
{
    const std::array<display::RgbColor, 1> pixel{display::RgbColor{.red = 255, .green = 0, .blue = 0}};
    const display::BitmapFile onscreen{
        .size = {.width = 1, .height = 1},
        .pixels = pixel,
    };
    const display::BitmapFile invalid{
        .size = {.width = 0, .height = 1},
        .pixels = pixel,
    };
    const display::BitmapFile undersized{
        .size = {.width = 2, .height = 1},
        .pixels = pixel,
    };

    display::LogicalFramebuffer framebuffer;
    framebuffer.clear(display::RgbColor{.red = 4, .green = 5, .blue = 6});
    const display::LogicalFramebuffer original = framebuffer;
    const display::BitmapRasterizer rasterizer;

    rasterizer.rasterize(onscreen, display::Position{.x = 32, .y = 0}, framebuffer);
    rasterizer.rasterize(onscreen, display::Position{.x = 0, .y = 8}, framebuffer);
    rasterizer.rasterize(onscreen, display::Position{.x = -1, .y = 0}, framebuffer);
    rasterizer.rasterize(onscreen, display::Position{.x = 0, .y = -1}, framebuffer);
    rasterizer.rasterize(invalid, display::Position{.x = 0, .y = 0}, framebuffer);
    rasterizer.rasterize(undersized, display::Position{.x = 0, .y = 0}, framebuffer);

    EXPECT_TRUE(framebuffers_equal(original, framebuffer));
}

TEST(BitmapRasterizer, RendersDifferentViewsThroughTheSameInstance)
{
    const std::array<display::RgbColor, 1> first_pixels{display::RgbColor{.red = 9, .green = 0, .blue = 0}};
    const std::array<display::RgbColor, 1> second_pixels{display::RgbColor{.red = 0, .green = 9, .blue = 0}};
    const display::BitmapFile first_view{
        .size = {.width = 1, .height = 1},
        .pixels = first_pixels,
    };
    const display::BitmapFile second_view{
        .size = {.width = 1, .height = 1},
        .pixels = second_pixels,
    };

    display::LogicalFramebuffer framebuffer;
    const display::BitmapRasterizer rasterizer;
    rasterizer.rasterize(first_view, display::Position{.x = 1, .y = 1}, framebuffer);
    expect_rgb_at(framebuffer, 1, 1, 9, 0, 0);

    framebuffer.clear();
    rasterizer.rasterize(second_view, display::Position{.x = 2, .y = 2}, framebuffer);
    expect_rgb_at(framebuffer, 2, 2, 0, 9, 0);
    expect_rgb_at(framebuffer, 1, 1, 0, 0, 0);
}
