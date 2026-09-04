#pragma once

#include "esp_log.h"

#include "platform/runtime_component.hpp"

#define PLATFORM_LOGD(component, fmt, ...) ESP_LOGD((component)->name(), fmt, ##__VA_ARGS__)
#define PLATFORM_LOGI(component, fmt, ...) ESP_LOGI((component)->name(), fmt, ##__VA_ARGS__)
#define PLATFORM_LOGW(component, fmt, ...) ESP_LOGW((component)->name(), fmt, ##__VA_ARGS__)
#define PLATFORM_LOGE(component, fmt, ...) ESP_LOGE((component)->name(), fmt, ##__VA_ARGS__)
