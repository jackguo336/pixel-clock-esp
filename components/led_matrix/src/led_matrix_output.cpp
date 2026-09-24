#include "led_matrix/led_matrix_output.hpp"

#include <cstdint>

#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led_strip.h"
#include "led_matrix_map.hpp"

namespace led_matrix {
namespace {

constexpr char kTag[] = "led_matrix";

[[nodiscard]] led_color_component_format_t grb_component_format()
{
    led_color_component_format_t format{};
    format.format.g_pos = 0;
    format.format.r_pos = 1;
    format.format.b_pos = 2;
    format.format.w_pos = 3;
    format.format.num_components = 3;
    return format;
}

}  // namespace

LedMatrixOutput::LedMatrixOutput(LedMatrixOutputConfig config) : config_(config) {}

LedMatrixOutput::~LedMatrixOutput()
{
    if (strip_ == nullptr) {
        return;
    }

    const esp_err_t err = led_strip_del(static_cast<led_strip_handle_t>(strip_));
    strip_ = nullptr;
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "led_strip_del failed: %s", esp_err_to_name(err));
    }
}

LedMatrixOutputStatus LedMatrixOutput::initialize()
{
    if (strip_ != nullptr) {
        return LedMatrixOutputStatus::Ok;
    }

    led_strip_config_t strip_config{};
    strip_config.strip_gpio_num = config_.gpio_num;
    strip_config.max_leds = static_cast<uint32_t>(display::LogicalFramebuffer::kPixelCount);
    strip_config.led_model = LED_MODEL_WS2812;
    strip_config.color_component_format = grb_component_format();
    strip_config.flags.invert_out = false;

    led_strip_spi_config_t spi_config{};
    spi_config.clk_src = SPI_CLK_SRC_DEFAULT;
    spi_config.spi_bus = SPI2_HOST;
    spi_config.flags.with_dma = true;

    led_strip_handle_t strip = nullptr;
    const esp_err_t err = led_strip_new_spi_device(&strip_config, &spi_config, &strip);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "led_strip_new_spi_device failed: %s", esp_err_to_name(err));
        return LedMatrixOutputStatus::DriverError;
    }

    strip_ = strip;
    return LedMatrixOutputStatus::Ok;
}

LedMatrixOutputStatus LedMatrixOutput::present(const display::LogicalFramebuffer& framebuffer)
{
    if (strip_ == nullptr) {
        return LedMatrixOutputStatus::NotInitialized;
    }

    auto* strip = static_cast<led_strip_handle_t>(strip_);
    for (int32_t y = 0; y < display::LogicalFramebuffer::kHeight; ++y) {
        for (int32_t x = 0; x < display::LogicalFramebuffer::kWidth; ++x) {
            const display::RgbColor* color = framebuffer.pixel_at(x, y);
            if (color == nullptr) {
                ESP_LOGE(kTag, "missing logical pixel x=%ld y=%ld", static_cast<long>(x),
                         static_cast<long>(y));
                return LedMatrixOutputStatus::DriverError;
            }

            const std::size_t index =
                led_index_for_logical_pixel(x, y, config_.orientation, config_.serpentine);
            const esp_err_t err = led_strip_set_pixel(strip, static_cast<uint32_t>(index), color->red,
                                                      color->green, color->blue);
            if (err != ESP_OK) {
                ESP_LOGE(kTag, "led_strip_set_pixel failed: %s", esp_err_to_name(err));
                return LedMatrixOutputStatus::DriverError;
            }
        }
    }

    const esp_err_t err = led_strip_refresh(strip);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "led_strip_refresh failed: %s", esp_err_to_name(err));
        return LedMatrixOutputStatus::DriverError;
    }
    return LedMatrixOutputStatus::Ok;
}

}  // namespace led_matrix
