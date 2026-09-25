#include <cstddef>
#include <cstdint>

#include "display/logical_framebuffer.hpp"
#include "gtest/gtest.h"
#include "led_matrix/led_matrix_output.hpp"
#include "mock_led_index.hpp"
#include "mock_led_strip.hpp"

namespace {

constexpr int kMatrixGpio = 3;

led_matrix::LedMatrixOutputConfig firmware_config()
{
    return led_matrix::LedMatrixOutputConfig{
        .gpio_num = kMatrixGpio,
        .orientation = led_matrix::LedMatrixOrientation::Horizontal,
        .serpentine = true,
    };
}

void fill_unique_colors(display::LogicalFramebuffer& framebuffer)
{
    framebuffer.clear();
    for (int32_t y = 0; y < display::LogicalFramebuffer::kHeight; ++y) {
        for (int32_t x = 0; x < display::LogicalFramebuffer::kWidth; ++x) {
            const display::RgbColor color{
                .red = static_cast<uint8_t>(x),
                .green = static_cast<uint8_t>(y),
                .blue = static_cast<uint8_t>(x + y),
            };
            ASSERT_TRUE(framebuffer.set_pixel(x, y, color));
        }
    }
}

void reset_mocks()
{
    mock_led_strip::reset();
    mock_led_index::reset();
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
    fill_unique_colors(framebuffer);

    led_matrix::LedMatrixOutput output(firmware_config());
    ASSERT_EQ(output.initialize(), led_matrix::LedMatrixOutputStatus::Ok);
    ASSERT_EQ(output.present(framebuffer), led_matrix::LedMatrixOutputStatus::Ok);

    EXPECT_EQ(mock_led_strip::set_pixel_count(), display::LogicalFramebuffer::kPixelCount);
    EXPECT_EQ(mock_led_strip::refresh_count(), 1u);
    EXPECT_EQ(mock_led_index::call_count(), display::LogicalFramebuffer::kPixelCount);

    std::size_t call = 0;
    for (int32_t y = 0; y < display::LogicalFramebuffer::kHeight; ++y) {
        for (int32_t x = 0; x < display::LogicalFramebuffer::kWidth; ++x) {
            const mock_led_strip::PixelWrite* write = mock_led_strip::pixel_write_at(call);
            ASSERT_NE(write, nullptr);
            const mock_led_index::Call* index_call = mock_led_index::call_at(call);
            ASSERT_NE(index_call, nullptr);
            EXPECT_EQ(index_call->x, x);
            EXPECT_EQ(index_call->y, y);
            EXPECT_EQ(index_call->orientation, led_matrix::LedMatrixOrientation::Horizontal);
            EXPECT_TRUE(index_call->serpentine);
            EXPECT_EQ(write->index, mock_led_index::index_for_call(call));
            EXPECT_EQ(write->red, static_cast<uint32_t>(x));
            EXPECT_EQ(write->green, static_cast<uint32_t>(y));
            EXPECT_EQ(write->blue, static_cast<uint32_t>(x + y));
            ++call;
        }
    }
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
