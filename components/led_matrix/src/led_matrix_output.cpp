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

// Full-scale value of one 8-bit color channel.
constexpr uint8_t kMaxColorChannelValue = 255;

// Map red, green, and blue from the full 0-255 range into 0-max_brightness
// using the same scale. Sharing the scale keeps the hue and only changes brightness,
// so a brighter shade of a color stays brighter inside the limited range.
[[nodiscard]] display::RgbColor color_within_brightness_limit(display::RgbColor color,
                                                              uint8_t max_brightness)
{
    const auto scale = [max_brightness](uint8_t channel) {
        return static_cast<uint8_t>(static_cast<uint16_t>(channel) * max_brightness /
                                    kMaxColorChannelValue);
    };

    return display::RgbColor{
        .red = scale(color.red),
        .green = scale(color.green),
        .blue = scale(color.blue),
    };
}

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
            const display::RgbColor safe_color =
                color_within_brightness_limit(*color, config_.max_brightness);
            const esp_err_t err = led_strip_set_pixel(strip, static_cast<uint32_t>(index), safe_color.red,
                                                      safe_color.green, safe_color.blue);
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
