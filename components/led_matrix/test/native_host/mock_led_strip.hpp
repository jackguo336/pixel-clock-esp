#pragma once

#include <cstddef>
#include <cstdint>

#include "led_strip.h"

namespace mock_led_strip {

struct PixelWrite {
    uint32_t index{0};
    uint32_t red{0};
    uint32_t green{0};
    uint32_t blue{0};
};

void reset();
void set_next_create_result(esp_err_t result);
void set_next_set_pixel_result(esp_err_t result);
void set_next_refresh_result(esp_err_t result);

bool created();
std::size_t set_pixel_count();
const PixelWrite* pixel_write_at(std::size_t index);
std::size_t refresh_count();
std::size_t delete_count();

}  // namespace mock_led_strip
