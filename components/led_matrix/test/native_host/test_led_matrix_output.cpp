#include <cstddef>
#include <cstdint>

#include "display/logical_framebuffer.hpp"
#include "gtest/gtest.h"
#include "led_matrix/led_matrix_output.hpp"
#include "mock_led_index.hpp"
#include "mock_led_strip.hpp"

namespace {

constexpr int kMatrixGpio = 3;

// Pinned here so these cases keep the same expected colors if the firmware
// brightness default changes.
constexpr uint8_t kTestMaxChannelBrightness = 5;
constexpr uint8_t kFullScaleChannelBrightness = 255;

led_matrix::LedMatrixOutputConfig firmware_config()
{
    return led_matrix::LedMatrixOutputConfig{
        .gpio_num = kMatrixGpio,
        .orientation = led_matrix::LedMatrixOrientation::Horizontal,
        .serpentine = true,
    };
}

void reset_mocks()
{
    mock_led_strip::reset();
    mock_led_index::reset();
}

void present_color(display::RgbColor color, const mock_led_strip::PixelWrite*& write,
                   uint8_t max_brightness = kTestMaxChannelBrightness)
{
    write = nullptr;
    reset_mocks();
    display::LogicalFramebuffer framebuffer;
    framebuffer.clear();
    ASSERT_TRUE(framebuffer.set_pixel(0, 0, color));

    led_matrix::LedMatrixOutputConfig config = firmware_config();
    config.max_brightness = max_brightness;
    led_matrix::LedMatrixOutput output(config);
    ASSERT_EQ(output.initialize(), led_matrix::LedMatrixOutputStatus::Ok);
    ASSERT_EQ(output.present(framebuffer), led_matrix::LedMatrixOutputStatus::Ok);

    write = mock_led_strip::pixel_write_at(0);
    ASSERT_NE(write, nullptr);
}

void expect_presented_color(display::RgbColor color, display::RgbColor expected,
                            uint8_t max_brightness = kTestMaxChannelBrightness)
{
    const mock_led_strip::PixelWrite* write = nullptr;
    present_color(color, write, max_brightness);
    ASSERT_NE(write, nullptr);
    EXPECT_EQ(write->red, static_cast<uint32_t>(expected.red));
    EXPECT_EQ(write->green, static_cast<uint32_t>(expected.green));
    EXPECT_EQ(write->blue, static_cast<uint32_t>(expected.blue));
}

void expect_mapped_pixel(int32_t x, int32_t y, display::RgbColor color)
{
    const std::size_t call = static_cast<std::size_t>(y) * display::LogicalFramebuffer::kWidth +
                             static_cast<std::size_t>(x);
    const mock_led_strip::PixelWrite* write = mock_led_strip::pixel_write_at(call);
    ASSERT_NE(write, nullptr);
    const mock_led_index::Call* index_call = mock_led_index::call_at(call);
    ASSERT_NE(index_call, nullptr);
    EXPECT_EQ(index_call->x, x);
    EXPECT_EQ(index_call->y, y);
    EXPECT_EQ(index_call->orientation, led_matrix::LedMatrixOrientation::Horizontal);
    EXPECT_TRUE(index_call->serpentine);
    EXPECT_EQ(write->index, mock_led_index::index_for_call(call));
    EXPECT_EQ(write->red, static_cast<uint32_t>(color.red));
    EXPECT_EQ(write->green, static_cast<uint32_t>(color.green));
    EXPECT_EQ(write->blue, static_cast<uint32_t>(color.blue));
}

}  // namespace

TEST(LedMatrixOutput, InitializesSuccessfully)
{
    reset_mocks();
    led_matrix::LedMatrixOutput output(firmware_config());

    EXPECT_EQ(output.initialize(), led_matrix::LedMatrixOutputStatus::Ok);
    EXPECT_EQ(mock_led_index::call_count(), 0u);
}

TEST(LedMatrixOutput, PresentWritesEachFramebufferColorToItsMappedIndexAndRefreshesOnce)
{
    reset_mocks();
    display::LogicalFramebuffer framebuffer;
    framebuffer.clear();
    constexpr display::RgbColor kTopLeft{.red = 1, .green = 2, .blue = 3};
    constexpr display::RgbColor kBottomRight{.red = 4, .green = 5, .blue = 6};
    ASSERT_TRUE(framebuffer.set_pixel(0, 0, kTopLeft));
    ASSERT_TRUE(framebuffer.set_pixel(display::LogicalFramebuffer::kWidth - 1,
                                      display::LogicalFramebuffer::kHeight - 1, kBottomRight));

    led_matrix::LedMatrixOutputConfig config = firmware_config();
    config.max_brightness = kFullScaleChannelBrightness;
    led_matrix::LedMatrixOutput output(config);
    ASSERT_EQ(output.initialize(), led_matrix::LedMatrixOutputStatus::Ok);
    ASSERT_EQ(output.present(framebuffer), led_matrix::LedMatrixOutputStatus::Ok);

    EXPECT_EQ(mock_led_strip::set_pixel_count(), display::LogicalFramebuffer::kPixelCount);
    EXPECT_EQ(mock_led_strip::refresh_count(), 1u);
    EXPECT_EQ(mock_led_index::call_count(), display::LogicalFramebuffer::kPixelCount);
    expect_mapped_pixel(0, 0, kTopLeft);
    expect_mapped_pixel(display::LogicalFramebuffer::kWidth - 1, display::LogicalFramebuffer::kHeight - 1,
                        kBottomRight);
}

TEST(LedMatrixOutput, PresentScalesWhiteSoEveryChannelMeetsTheBrightnessLimit)
{
    expect_presented_color({.red = 255, .green = 255, .blue = 255},
                           {.red = kTestMaxChannelBrightness,
                            .green = kTestMaxChannelBrightness,
                            .blue = kTestMaxChannelBrightness});
}

TEST(LedMatrixOutput, PresentScalesColorWithSomeChannelsOverTheBrightnessLimit)
{
    // 128 * 5 / 255 = 2. Half of full-scale green stays half of the limited red.
    expect_presented_color({.red = 255, .green = 128, .blue = 0},
                           {.red = kTestMaxChannelBrightness, .green = 2, .blue = 0});
}

TEST(LedMatrixOutput, PresentLeavesBlackUnchanged)
{
    expect_presented_color({.red = 0, .green = 0, .blue = 0}, {.red = 0, .green = 0, .blue = 0});
}

TEST(LedMatrixOutput, PresentBeforeInitializeFailsWithoutTouchingTheDriver)
{
    reset_mocks();
    const display::LogicalFramebuffer framebuffer;
    led_matrix::LedMatrixOutput output(firmware_config());

    EXPECT_EQ(output.present(framebuffer), led_matrix::LedMatrixOutputStatus::NotInitialized);
    EXPECT_FALSE(mock_led_strip::created());
    EXPECT_EQ(mock_led_strip::set_pixel_count(), 0u);
    EXPECT_EQ(mock_led_strip::refresh_count(), 0u);
    EXPECT_EQ(mock_led_index::call_count(), 0u);
}

TEST(LedMatrixOutput, InitializeFailurePropagatesAndLeavesOutputUninitialized)
{
    reset_mocks();
    mock_led_strip::set_next_create_result(ESP_FAIL);
    {
        led_matrix::LedMatrixOutput output(firmware_config());
        EXPECT_EQ(output.initialize(), led_matrix::LedMatrixOutputStatus::DriverError);
        EXPECT_EQ(output.present(display::LogicalFramebuffer{}), led_matrix::LedMatrixOutputStatus::NotInitialized);
    }
    EXPECT_EQ(mock_led_strip::delete_count(), 0u);
    EXPECT_EQ(mock_led_index::call_count(), 0u);
}

TEST(LedMatrixOutput, SetPixelFailurePropagatesWithoutRefreshing)
{
    reset_mocks();
    mock_led_strip::set_next_set_pixel_result(ESP_FAIL);
    led_matrix::LedMatrixOutput output(firmware_config());

    ASSERT_EQ(output.initialize(), led_matrix::LedMatrixOutputStatus::Ok);
    EXPECT_EQ(output.present(display::LogicalFramebuffer{}), led_matrix::LedMatrixOutputStatus::DriverError);
    EXPECT_EQ(mock_led_strip::refresh_count(), 0u);
    EXPECT_EQ(mock_led_index::call_count(), 1u);
}

TEST(LedMatrixOutput, RefreshFailurePropagatesAfterWritingEveryPixel)
{
    reset_mocks();
    mock_led_strip::set_next_refresh_result(ESP_FAIL);
    led_matrix::LedMatrixOutput output(firmware_config());

    ASSERT_EQ(output.initialize(), led_matrix::LedMatrixOutputStatus::Ok);
    EXPECT_EQ(output.present(display::LogicalFramebuffer{}), led_matrix::LedMatrixOutputStatus::DriverError);
    EXPECT_EQ(mock_led_strip::refresh_count(), 1u);
    EXPECT_EQ(mock_led_index::call_count(), display::LogicalFramebuffer::kPixelCount);
}

TEST(LedMatrixOutput, DestructionReleasesTheStripHandle)
{
    reset_mocks();
    {
        led_matrix::LedMatrixOutput output(firmware_config());
        ASSERT_EQ(output.initialize(), led_matrix::LedMatrixOutputStatus::Ok);
        EXPECT_EQ(mock_led_strip::delete_count(), 0u);
    }
    EXPECT_EQ(mock_led_strip::delete_count(), 1u);
    EXPECT_FALSE(mock_led_strip::created());
}
