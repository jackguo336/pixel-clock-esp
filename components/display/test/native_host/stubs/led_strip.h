#pragma once

#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct led_strip_t* led_strip_handle_t;

typedef enum {
    LED_MODEL_WS2812,
} led_model_t;

struct led_color_component_positions_t {
    uint32_t g_pos;
    uint32_t r_pos;
    uint32_t b_pos;
    uint32_t w_pos;
    uint32_t num_components;
};

typedef struct {
    led_color_component_positions_t format;
} led_color_component_format_t;

struct led_strip_config_flags_t {
    uint32_t invert_out;
};

typedef struct {
    int strip_gpio_num;
    uint32_t max_leds;
    led_model_t led_model;
    led_color_component_format_t color_component_format;
    led_strip_config_flags_t flags;
} led_strip_config_t;

struct led_strip_spi_config_flags_t {
    uint32_t with_dma;
};

typedef struct {
    spi_clock_source_t clk_src;
    spi_host_device_t spi_bus;
    led_strip_spi_config_flags_t flags;
} led_strip_spi_config_t;

esp_err_t led_strip_new_spi_device(const led_strip_config_t* led_config,
                                   const led_strip_spi_config_t* spi_config,
                                   led_strip_handle_t* ret_strip);
esp_err_t led_strip_set_pixel(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green,
                              uint32_t blue);
esp_err_t led_strip_refresh(led_strip_handle_t strip);
esp_err_t led_strip_del(led_strip_handle_t strip);

#ifdef __cplusplus
}
#endif
