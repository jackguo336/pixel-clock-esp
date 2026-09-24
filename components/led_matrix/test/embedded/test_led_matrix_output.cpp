#include "unity.h"

#include <cstddef>
#include <cstdint>

#include "display/logical_framebuffer.hpp"
#include "led_matrix/led_matrix_output.hpp"
#include "led_matrix_map.hpp"
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
            TEST_ASSERT_TRUE(framebuffer.set_pixel(x, y, color));
        }
    }
}

}  // namespace

TEST_CASE("initialize configures a 256-pixel WS2812 GRB strip on GPIO 3 via SPI2 DMA", "[led_matrix]")
{
    mock_led_strip::reset();
    led_matrix::LedMatrixOutput output(firmware_config());

    TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::Ok),
                      static_cast<int>(output.initialize()));

    const led_strip_config_t* strip = mock_led_strip::strip_config();
    const led_strip_spi_config_t* spi = mock_led_strip::spi_config();
    TEST_ASSERT_NOT_NULL(strip);
    TEST_ASSERT_NOT_NULL(spi);
    TEST_ASSERT_EQUAL_INT(kMatrixGpio, strip->strip_gpio_num);
    TEST_ASSERT_EQUAL_UINT32(display::LogicalFramebuffer::kPixelCount, strip->max_leds);
    TEST_ASSERT_EQUAL(LED_MODEL_WS2812, strip->led_model);
    TEST_ASSERT_EQUAL_UINT(0, strip->color_component_format.format.g_pos);
    TEST_ASSERT_EQUAL_UINT(1, strip->color_component_format.format.r_pos);
    TEST_ASSERT_EQUAL_UINT(2, strip->color_component_format.format.b_pos);
    TEST_ASSERT_EQUAL_UINT(3, strip->color_component_format.format.num_components);
    TEST_ASSERT_EQUAL(SPI2_HOST, spi->spi_bus);
    TEST_ASSERT_EQUAL(SPI_CLK_SRC_DEFAULT, spi->clk_src);
    TEST_ASSERT_TRUE(spi->flags.with_dma);
}

TEST_CASE("present writes each framebuffer color to its mapped index and refreshes once", "[led_matrix]")
{
    mock_led_strip::reset();
    display::LogicalFramebuffer framebuffer;
    fill_unique_colors(framebuffer);

    led_matrix::LedMatrixOutput output(firmware_config());
    TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::Ok),
                      static_cast<int>(output.initialize()));
    TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::Ok),
                      static_cast<int>(output.present(framebuffer)));

    TEST_ASSERT_EQUAL_UINT(display::LogicalFramebuffer::kPixelCount, mock_led_strip::set_pixel_count());
    TEST_ASSERT_EQUAL_UINT(1, mock_led_strip::refresh_count());

    std::size_t call = 0;
    for (int32_t y = 0; y < display::LogicalFramebuffer::kHeight; ++y) {
        for (int32_t x = 0; x < display::LogicalFramebuffer::kWidth; ++x) {
            const mock_led_strip::PixelWrite* write = mock_led_strip::pixel_write_at(call);
            TEST_ASSERT_NOT_NULL(write);
            const std::size_t expected_index = led_matrix::led_index_for_logical_pixel(
                x, y, led_matrix::LedMatrixOrientation::Horizontal, true);
            TEST_ASSERT_EQUAL_UINT32(expected_index, write->index);
            TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(x), write->red);
            TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(y), write->green);
            TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(x + y), write->blue);
            ++call;
        }
    }

    const mock_led_strip::PixelWrite* top_left = mock_led_strip::pixel_write_at(0);
    TEST_ASSERT_NOT_NULL(top_left);
    TEST_ASSERT_EQUAL_UINT32(0, top_left->index);
    const mock_led_strip::PixelWrite* odd_row_start = mock_led_strip::pixel_write_at(
        static_cast<std::size_t>(display::LogicalFramebuffer::kWidth));
    TEST_ASSERT_NOT_NULL(odd_row_start);
    TEST_ASSERT_EQUAL_UINT32(63, odd_row_start->index);
}

TEST_CASE("present before initialize fails without touching the driver", "[led_matrix]")
{
    mock_led_strip::reset();
    const display::LogicalFramebuffer framebuffer;
    led_matrix::LedMatrixOutput output(firmware_config());

    TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::NotInitialized),
                      static_cast<int>(output.present(framebuffer)));
    TEST_ASSERT_FALSE(mock_led_strip::created());
    TEST_ASSERT_EQUAL_UINT(0, mock_led_strip::set_pixel_count());
    TEST_ASSERT_EQUAL_UINT(0, mock_led_strip::refresh_count());
}

TEST_CASE("driver failures during initialize set and refresh propagate", "[led_matrix]")
{
    mock_led_strip::reset();
    mock_led_strip::set_next_create_result(ESP_FAIL);
    {
        led_matrix::LedMatrixOutput output(firmware_config());
        TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::DriverError),
                          static_cast<int>(output.initialize()));
        TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::NotInitialized),
                          static_cast<int>(output.present(display::LogicalFramebuffer{})));
    }
    TEST_ASSERT_EQUAL_UINT(0, mock_led_strip::delete_count());

    mock_led_strip::reset();
    mock_led_strip::set_next_set_pixel_result(ESP_FAIL);
    {
        led_matrix::LedMatrixOutput output(firmware_config());
        TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::Ok),
                      static_cast<int>(output.initialize()));
        TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::DriverError),
                          static_cast<int>(output.present(display::LogicalFramebuffer{})));
        TEST_ASSERT_EQUAL_UINT(0, mock_led_strip::refresh_count());
    }

    mock_led_strip::reset();
    mock_led_strip::set_next_refresh_result(ESP_FAIL);
    {
        led_matrix::LedMatrixOutput output(firmware_config());
        TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::Ok),
                      static_cast<int>(output.initialize()));
        TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::DriverError),
                          static_cast<int>(output.present(display::LogicalFramebuffer{})));
        TEST_ASSERT_EQUAL_UINT(1, mock_led_strip::refresh_count());
    }
}

TEST_CASE("destruction releases the strip handle", "[led_matrix]")
{
    mock_led_strip::reset();
    {
        led_matrix::LedMatrixOutput output(firmware_config());
        TEST_ASSERT_EQUAL(static_cast<int>(led_matrix::LedMatrixOutputStatus::Ok),
                      static_cast<int>(output.initialize()));
        TEST_ASSERT_EQUAL_UINT(0, mock_led_strip::delete_count());
    }
    TEST_ASSERT_EQUAL_UINT(1, mock_led_strip::delete_count());
    TEST_ASSERT_FALSE(mock_led_strip::created());
}
