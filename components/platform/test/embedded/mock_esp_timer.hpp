#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "esp_timer.h"

namespace mock_esp_timer {

struct Timer {
    bool allocated{false};
    bool running{false};
    bool periodic{false};
    uint64_t interval_us{0};
    esp_timer_cb_t callback{nullptr};
    void* arg{nullptr};
    esp_timer_dispatch_t dispatch_method{ESP_TIMER_TASK};
    const char* name{nullptr};
    bool skip_unhandled_events{false};
};

// Directs subsequent esp_timer_create calls into the fake. Other wrapped
// functions still forward real handles to IDF so boot-time timers keep working.
void reset();
void restore();

void set_next_create_result(esp_err_t result);
void set_next_start_result(esp_err_t result);

std::size_t create_count();
std::size_t delete_count();
std::size_t start_once_count();
std::size_t start_periodic_count();
std::size_t stop_count();
std::size_t allocated_count();

const Timer* timer_at(std::size_t index);

bool fire(std::size_t index);
bool invoke(std::size_t index);

}  // namespace mock_esp_timer
