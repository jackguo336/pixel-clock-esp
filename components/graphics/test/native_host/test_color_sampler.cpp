#include <cstddef>
#include <initializer_list>

#include "color_sampler.hpp"
#include "gtest/gtest.h"
#include "utils.hpp"

using graphics_test::expect_rgb;

namespace {

[[nodiscard]] graphics::LinearGradientPaint gradient_paint(
    float angle_degrees, std::initializer_list<graphics::GradientColorStop> stops)
{
    graphics::LinearGradientPaint paint{};
    paint.angle_degrees = angle_degrees;
    paint.color_stop_count = static_cast<uint8_t>(stops.size());
    std::size_t index = 0;
    for (const graphics::GradientColorStop& stop : stops) {
        paint.color_stops[index] = stop;
        ++index;
    }
    return paint;
}

[[nodiscard]] graphics::ColorSampler sampler(float angle_degrees,
                                             std::initializer_list<graphics::GradientColorStop> stops,
                                             graphics::Position origin, graphics::Size size)
{
    return graphics::ColorSampler{graphics::Paint{gradient_paint(angle_degrees, stops)}, origin, size};
}

const graphics::GradientColorStop kRed{.offset = 0.0f, .color = {.red = 255, .green = 0, .blue = 0}};
const graphics::GradientColorStop kBlue{.offset = 1.0f, .color = {.red = 0, .green = 0, .blue = 255}};

}  // namespace

TEST(ColorSampler, SolidPaintIgnoresPositionAndBounds)
{
    const graphics::ColorSampler solid{
        graphics::Paint{graphics::SolidPaint{.color = {.red = 1, .green = 2, .blue = 3}}},
        graphics::Position{.x = 4, .y = 5},
        graphics::Size{.width = 6, .height = 7},
    };

    expect_rgb(solid.sample({.x = 0, .y = 0}), 1, 2, 3);
    expect_rgb(solid.sample({.x = 4, .y = 5}), 1, 2, 3);
    expect_rgb(solid.sample({.x = -8, .y = 20}), 1, 2, 3);
}

TEST(ColorSampler, CardinalAnglesCoverTranslatedNonSquareBounds)
{
    const graphics::Position origin{.x = 5, .y = 7};
    const graphics::Size size{.width = 4, .height = 2};
    const graphics::ColorSampler right = sampler(90.0f, {kRed, kBlue}, origin, size);
    expect_rgb(right.sample({.x = 5, .y = 7}), 255, 0, 0);
    expect_rgb(right.sample({.x = 5, .y = 8}), 255, 0, 0);
    expect_rgb(right.sample({.x = 6, .y = 8}), 191, 0, 64);
    expect_rgb(right.sample({.x = 7, .y = 7}), 128, 0, 128);
    expect_rgb(right.sample({.x = 7, .y = 9}), 128, 0, 128);
    expect_rgb(right.sample({.x = 8, .y = 7}), 64, 0, 191);
    expect_rgb(right.sample({.x = 9, .y = 7}), 0, 0, 255);
    expect_rgb(right.sample({.x = 4, .y = 7}), 255, 0, 0);
    expect_rgb(right.sample({.x = 12, .y = 8}), 0, 0, 255);

    const graphics::ColorSampler up = sampler(0.0f, {kRed, kBlue}, origin, size);
    expect_rgb(up.sample({.x = 5, .y = 7}), 0, 0, 255);
    expect_rgb(up.sample({.x = 8, .y = 8}), 128, 0, 128);
    expect_rgb(up.sample({.x = 6, .y = 9}), 255, 0, 0);

    const graphics::ColorSampler down = sampler(180.0f, {kRed, kBlue}, origin, size);
    expect_rgb(down.sample({.x = 5, .y = 7}), 255, 0, 0);
    expect_rgb(down.sample({.x = 8, .y = 8}), 128, 0, 128);
    expect_rgb(down.sample({.x = 6, .y = 9}), 0, 0, 255);

    const graphics::ColorSampler left = sampler(270.0f, {kRed, kBlue}, origin, size);
    expect_rgb(left.sample({.x = 5, .y = 8}), 0, 0, 255);
    expect_rgb(left.sample({.x = 6, .y = 7}), 64, 0, 191);
    expect_rgb(left.sample({.x = 7, .y = 9}), 128, 0, 128);
    expect_rgb(left.sample({.x = 8, .y = 7}), 191, 0, 64);
    expect_rgb(left.sample({.x = 9, .y = 8}), 255, 0, 0);

    const graphics::ColorSampler negative_right = sampler(-270.0f, {kRed, kBlue}, origin, size);
    expect_rgb(negative_right.sample({.x = 6, .y = 8}), 191, 0, 64);
    expect_rgb(negative_right.sample({.x = 9, .y = 7}), 0, 0, 255);
}

TEST(ColorSampler, DiagonalAnglesProjectBothAxesOfThePaintBounds)
{
    const graphics::Position origin{.x = 10, .y = 20};
    const graphics::Size size{.width = 4, .height = 2};
    const graphics::ColorSampler diagonal = sampler(45.0f, {kRed, kBlue}, origin, size);

    expect_rgb(diagonal.sample({.x = 10, .y = 22}), 255, 0, 0);
    expect_rgb(diagonal.sample({.x = 10, .y = 20}), 170, 0, 85);
    expect_rgb(diagonal.sample({.x = 14, .y = 20}), 0, 0, 255);

    const graphics::ColorSampler square = sampler(45.0f, {kRed, kBlue}, {}, {.width = 2, .height = 2});
    expect_rgb(square.sample({.x = 0, .y = 2}), 255, 0, 0);
    expect_rgb(square.sample({.x = 0, .y = 1}), 191, 0, 64);
    expect_rgb(square.sample({.x = 1, .y = 0}), 64, 0, 191);
    expect_rgb(square.sample({.x = 2, .y = 0}), 0, 0, 255);

    const graphics::ColorSampler down_right = sampler(135.0f, {kRed, kBlue}, {}, {.width = 4, .height = 2});
    expect_rgb(down_right.sample({.x = 0, .y = 0}), 255, 0, 0);
    expect_rgb(down_right.sample({.x = 4, .y = 2}), 0, 0, 255);
}

TEST(ColorSampler, InterpolatesBetweenSurroundingStops)
{
    const graphics::ColorSampler stops = sampler(
        90.0f,
        {
            graphics::GradientColorStop{.offset = 0.0f, .color = {.red = 0, .green = 0, .blue = 0}},
            graphics::GradientColorStop{.offset = 0.25f, .color = {.red = 255, .green = 0, .blue = 0}},
            graphics::GradientColorStop{.offset = 0.5f, .color = {.red = 0, .green = 255, .blue = 0}},
            graphics::GradientColorStop{.offset = 1.0f, .color = {.red = 0, .green = 0, .blue = 255}},
        },
        {},
        {.width = 8, .height = 1});

    expect_rgb(stops.sample({.x = 0, .y = 0}), 0, 0, 0);
    expect_rgb(stops.sample({.x = 1, .y = 0}), 128, 0, 0);
    expect_rgb(stops.sample({.x = 2, .y = 0}), 255, 0, 0);
    expect_rgb(stops.sample({.x = 3, .y = 0}), 128, 128, 0);
    expect_rgb(stops.sample({.x = 4, .y = 0}), 0, 255, 0);
    expect_rgb(stops.sample({.x = 6, .y = 0}), 0, 128, 128);
    expect_rgb(stops.sample({.x = 7, .y = 0}), 0, 64, 191);
}

TEST(ColorSampler, ClampsOutsideTheFirstAndLastStop)
{
    const graphics::ColorSampler clamped = sampler(
        90.0f,
        {
            graphics::GradientColorStop{.offset = 0.25f, .color = {.red = 255, .green = 0, .blue = 0}},
            graphics::GradientColorStop{.offset = 0.75f, .color = {.red = 0, .green = 0, .blue = 255}},
        },
        {},
        {.width = 4, .height = 1});

    expect_rgb(clamped.sample({.x = -2, .y = 0}), 255, 0, 0);
    expect_rgb(clamped.sample({.x = 0, .y = 0}), 255, 0, 0);
    expect_rgb(clamped.sample({.x = 1, .y = 0}), 255, 0, 0);
    expect_rgb(clamped.sample({.x = 2, .y = 0}), 128, 0, 128);
    expect_rgb(clamped.sample({.x = 3, .y = 0}), 0, 0, 255);
    expect_rgb(clamped.sample({.x = 4, .y = 0}), 0, 0, 255);
}

TEST(ColorSampler, ZeroStopsAreBlackAndOneStopIsConstant)
{
    const graphics::ColorSampler empty{graphics::LinearGradientPaint{}, graphics::Position{.x = 1, .y = 1},
                                       graphics::Size{.width = 4, .height = 4}};
    expect_rgb(empty.sample({.x = 1, .y = 1}), 0, 0, 0);
    expect_rgb(empty.sample({.x = 3, .y = 2}), 0, 0, 0);

    const graphics::ColorSampler single = sampler(
        37.0f,
        {
            graphics::GradientColorStop{.offset = 0.4f, .color = {.red = 9, .green = 8, .blue = 7}},
        },
        {},
        {.width = 5, .height = 5});
    expect_rgb(single.sample({.x = 0, .y = 0}), 9, 8, 7);
    expect_rgb(single.sample({.x = 4, .y = 4}), 9, 8, 7);
    expect_rgb(single.sample({.x = -3, .y = 9}), 9, 8, 7);
}

TEST(ColorSampler, OnePixelBoundsUseTheCornerTheAnglePointsToward)
{
    const graphics::Position origin{.x = 5, .y = 3};
    const graphics::Size one_pixel{.width = 1, .height = 1};
    const graphics::Position sample_at = origin;

    expect_rgb(sampler(90.0f, {kRed, kBlue}, origin, one_pixel).sample(sample_at), 255, 0, 0);
    expect_rgb(sampler(180.0f, {kRed, kBlue}, origin, one_pixel).sample(sample_at), 255, 0, 0);
    expect_rgb(sampler(0.0f, {kRed, kBlue}, origin, one_pixel).sample(sample_at), 0, 0, 255);
    expect_rgb(sampler(270.0f, {kRed, kBlue}, origin, one_pixel).sample(sample_at), 0, 0, 255);
    expect_rgb(sampler(45.0f, {kRed, kBlue}, {}, one_pixel).sample({.x = 0, .y = 0}), 127, 0, 128);
}
