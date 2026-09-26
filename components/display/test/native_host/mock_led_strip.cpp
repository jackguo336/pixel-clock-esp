#include "mock_led_strip.hpp"

#include <array>

#include "logical_framebuffer.hpp"

namespace mock_led_strip {
namespace {

constexpr std::size_t kPixelCapacity = display::LogicalFramebuffer::kPixelCount;

int g_handle_sentinel{0};
bool g_created{false};
std::array<PixelWrite, kPixelCapacity> g_pixel_writes{};
std::size_t g_set_pixel_count{0};
std::size_t g_refresh_count{0};
std::size_t g_delete_count{0};
esp_err_t g_next_create_result{ESP_OK};
esp_err_t g_next_set_pixel_result{ESP_OK};
esp_err_t g_next_refresh_result{ESP_OK};

}  // namespace

void reset()
{
    g_created = false;
    g_pixel_writes.fill(PixelWrite{});
    g_set_pixel_count = 0;
    g_refresh_count = 0;
    g_delete_count = 0;
    g_next_create_result = ESP_OK;
    g_next_set_pixel_result = ESP_OK;
    g_next_refresh_result = ESP_OK;
}

void set_next_create_result(esp_err_t result)
{
    g_next_create_result = result;
}

void set_next_set_pixel_result(esp_err_t result)
{
    g_next_set_pixel_result = result;
}

void set_next_refresh_result(esp_err_t result)
{
    g_next_refresh_result = result;
}

bool created()
{
    return g_created;
}

std::size_t set_pixel_count()
{
    return g_set_pixel_count;
}

const PixelWrite* pixel_write_at(std::size_t index)
{
    if (index >= g_set_pixel_count || index >= kPixelCapacity) {
        return nullptr;
    }
    return &g_pixel_writes[index];
}

std::size_t refresh_count()
{
    return g_refresh_count;
}

std::size_t delete_count()
{
    return g_delete_count;
}

esp_err_t create_device(const led_strip_config_t* led_config, const led_strip_spi_config_t* spi_config,
                        led_strip_handle_t* ret_strip)
{
    const esp_err_t forced = g_next_create_result;
    g_next_create_result = ESP_OK;
    if (forced != ESP_OK) {
        return forced;
    }
    if (led_config == nullptr || spi_config == nullptr || ret_strip == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    g_created = true;
    *ret_strip = reinterpret_cast<led_strip_handle_t>(&g_handle_sentinel);
    return ESP_OK;
}

esp_err_t set_pixel(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    if (strip != reinterpret_cast<led_strip_handle_t>(&g_handle_sentinel)) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t forced = g_next_set_pixel_result;
    g_next_set_pixel_result = ESP_OK;
    if (forced != ESP_OK) {
        return forced;
    }
    if (g_set_pixel_count >= kPixelCapacity) {
        return ESP_ERR_INVALID_SIZE;
    }
    g_pixel_writes[g_set_pixel_count] = PixelWrite{
        .index = index,
        .red = red,
        .green = green,
        .blue = blue,
    };
    ++g_set_pixel_count;
    return ESP_OK;
}

esp_err_t refresh(led_strip_handle_t strip)
{
    if (strip != reinterpret_cast<led_strip_handle_t>(&g_handle_sentinel)) {
        return ESP_ERR_INVALID_ARG;
    }
    ++g_refresh_count;
    const esp_err_t forced = g_next_refresh_result;
    g_next_refresh_result = ESP_OK;
    return forced;
}

esp_err_t release(led_strip_handle_t strip)
{
    if (strip != reinterpret_cast<led_strip_handle_t>(&g_handle_sentinel)) {
        return ESP_ERR_INVALID_ARG;
    }
    ++g_delete_count;
    g_created = false;
    return ESP_OK;
}

}  // namespace mock_led_strip

extern "C" {

esp_err_t led_strip_new_spi_device(const led_strip_config_t* led_config, const led_strip_spi_config_t* spi_config,
                                   led_strip_handle_t* ret_strip)
{
    return mock_led_strip::create_device(led_config, spi_config, ret_strip);
}

esp_err_t led_strip_set_pixel(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    return mock_led_strip::set_pixel(strip, index, red, green, blue);
}

esp_err_t led_strip_refresh(led_strip_handle_t strip)
{
    return mock_led_strip::refresh(strip);
}

esp_err_t led_strip_del(led_strip_handle_t strip)
{
    return mock_led_strip::release(strip);
}

}  // extern "C"
