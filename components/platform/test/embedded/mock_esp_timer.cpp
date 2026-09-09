#include "mock_esp_timer.hpp"

#include <cstdint>

namespace mock_esp_timer {
namespace {

constexpr std::size_t kCapacity = 16;

Timer g_timers[kCapacity]{};
bool g_intercept_create{false};
esp_err_t g_next_create_result{ESP_OK};
esp_err_t g_next_start_result{ESP_OK};
std::size_t g_create_count{0};
std::size_t g_delete_count{0};
std::size_t g_start_once_count{0};
std::size_t g_start_periodic_count{0};
std::size_t g_stop_count{0};

void clear_state()
{
    for (std::size_t i = 0; i < kCapacity; ++i) {
        g_timers[i] = Timer{};
    }
    g_next_create_result = ESP_OK;
    g_next_start_result = ESP_OK;
    g_create_count = 0;
    g_delete_count = 0;
    g_start_once_count = 0;
    g_start_periodic_count = 0;
    g_stop_count = 0;
}

Timer* as_fake(esp_timer_handle_t timer)
{
    if (timer == nullptr) {
        return nullptr;
    }
    const uintptr_t addr = reinterpret_cast<uintptr_t>(timer);
    const uintptr_t begin = reinterpret_cast<uintptr_t>(g_timers);
    if (addr < begin) {
        return nullptr;
    }
    const uintptr_t offset = addr - begin;
    if (offset % sizeof(Timer) != 0) {
        return nullptr;
    }
    const std::size_t index = static_cast<std::size_t>(offset / sizeof(Timer));
    if (index >= kCapacity) {
        return nullptr;
    }
    Timer* slot = &g_timers[index];
    if (!slot->allocated) {
        return nullptr;
    }
    return slot;
}

Timer* find_free_slot()
{
    for (std::size_t i = 0; i < kCapacity; ++i) {
        if (!g_timers[i].allocated) {
            return &g_timers[i];
        }
    }
    return nullptr;
}

esp_err_t consume_start_result(Timer& slot, bool periodic, uint64_t interval_us)
{
    ++(periodic ? g_start_periodic_count : g_start_once_count);
    const esp_err_t result = g_next_start_result;
    g_next_start_result = ESP_OK;
    if (result != ESP_OK) {
        return result;
    }
    if (slot.running) {
        return ESP_ERR_INVALID_STATE;
    }
    slot.running = true;
    slot.periodic = periodic;
    slot.interval_us = interval_us;
    return ESP_OK;
}

esp_err_t create_fake(const esp_timer_create_args_t* create_args, esp_timer_handle_t* out_handle)
{
    ++g_create_count;
    const esp_err_t forced = g_next_create_result;
    g_next_create_result = ESP_OK;
    if (forced != ESP_OK) {
        return forced;
    }
    if (create_args == nullptr || out_handle == nullptr || create_args->callback == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    Timer* slot = find_free_slot();
    if (slot == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    *slot = Timer{};
    slot->allocated = true;
    slot->callback = create_args->callback;
    slot->arg = create_args->arg;
    slot->dispatch_method = create_args->dispatch_method;
    slot->name = create_args->name;
    slot->skip_unhandled_events = create_args->skip_unhandled_events;
    *out_handle = reinterpret_cast<esp_timer_handle_t>(slot);
    return ESP_OK;
}

}  // namespace

void reset()
{
    clear_state();
    g_intercept_create = true;
}

void restore()
{
    g_intercept_create = false;
    clear_state();
}

void set_next_create_result(esp_err_t result)
{
    g_next_create_result = result;
}

void set_next_start_result(esp_err_t result)
{
    g_next_start_result = result;
}

std::size_t create_count()
{
    return g_create_count;
}

std::size_t delete_count()
{
    return g_delete_count;
}

std::size_t start_once_count()
{
    return g_start_once_count;
}

std::size_t start_periodic_count()
{
    return g_start_periodic_count;
}

std::size_t stop_count()
{
    return g_stop_count;
}

std::size_t allocated_count()
{
    std::size_t count = 0;
    for (std::size_t i = 0; i < kCapacity; ++i) {
        if (g_timers[i].allocated) {
            ++count;
        }
    }
    return count;
}

const Timer* timer_at(std::size_t index)
{
    if (index >= kCapacity || !g_timers[index].allocated) {
        return nullptr;
    }
    return &g_timers[index];
}

bool invoke(std::size_t index)
{
    const Timer* slot = timer_at(index);
    if (slot == nullptr || slot->callback == nullptr) {
        return false;
    }
    slot->callback(slot->arg);
    return true;
}

bool fire(std::size_t index)
{
    Timer* slot = index < kCapacity ? &g_timers[index] : nullptr;
    if (slot == nullptr || !slot->allocated || !slot->running) {
        return false;
    }
    if (!slot->periodic) {
        slot->running = false;
    }
    if (slot->callback == nullptr) {
        return false;
    }
    slot->callback(slot->arg);
    return true;
}

bool intercepting_create()
{
    return g_intercept_create;
}

Timer* fake_from_handle(esp_timer_handle_t timer)
{
    return as_fake(timer);
}

esp_err_t create_intercepted(const esp_timer_create_args_t* create_args,
                             esp_timer_handle_t* out_handle)
{
    return create_fake(create_args, out_handle);
}

esp_err_t start_intercepted(Timer& slot, bool periodic, uint64_t interval_us)
{
    return consume_start_result(slot, periodic, interval_us);
}

void note_stop(Timer& slot)
{
    ++g_stop_count;
    slot.running = false;
}

void note_delete(Timer& slot)
{
    ++g_delete_count;
    slot = Timer{};
}

}  // namespace mock_esp_timer

extern "C" {

esp_err_t __real_esp_timer_create(const esp_timer_create_args_t* create_args,
                                  esp_timer_handle_t* out_handle);
esp_err_t __real_esp_timer_delete(esp_timer_handle_t timer);
esp_err_t __real_esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us);
esp_err_t __real_esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period);
esp_err_t __real_esp_timer_stop(esp_timer_handle_t timer);

esp_err_t __wrap_esp_timer_create(const esp_timer_create_args_t* create_args,
                                  esp_timer_handle_t* out_handle)
{
    if (!mock_esp_timer::intercepting_create()) {
        return __real_esp_timer_create(create_args, out_handle);
    }
    return mock_esp_timer::create_intercepted(create_args, out_handle);
}

esp_err_t __wrap_esp_timer_delete(esp_timer_handle_t timer)
{
    mock_esp_timer::Timer* slot = mock_esp_timer::fake_from_handle(timer);
    if (slot == nullptr) {
        return __real_esp_timer_delete(timer);
    }
    mock_esp_timer::note_delete(*slot);
    return ESP_OK;
}

esp_err_t __wrap_esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us)
{
    mock_esp_timer::Timer* slot = mock_esp_timer::fake_from_handle(timer);
    if (slot == nullptr) {
        return __real_esp_timer_start_once(timer, timeout_us);
    }
    return mock_esp_timer::start_intercepted(*slot, false, timeout_us);
}

esp_err_t __wrap_esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period)
{
    mock_esp_timer::Timer* slot = mock_esp_timer::fake_from_handle(timer);
    if (slot == nullptr) {
        return __real_esp_timer_start_periodic(timer, period);
    }
    return mock_esp_timer::start_intercepted(*slot, true, period);
}

esp_err_t __wrap_esp_timer_stop(esp_timer_handle_t timer)
{
    mock_esp_timer::Timer* slot = mock_esp_timer::fake_from_handle(timer);
    if (slot == nullptr) {
        return __real_esp_timer_stop(timer);
    }
    mock_esp_timer::note_stop(*slot);
    return ESP_OK;
}

}  // extern "C"
