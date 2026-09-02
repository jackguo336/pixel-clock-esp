#include "platform/scheduler.hpp"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "freertos_handles.hpp"
#include "platform/runtime.hpp"

namespace platform {
namespace {

constexpr char kTag[] = "scheduler";

}  // namespace

struct Scheduler::TimerSlot {
    bool in_use{false};
    bool periodic{false};
    uint32_t id{0};
    Event event{};
    esp_timer_handle_t timer{nullptr};
    Scheduler* owner{nullptr};
};

Scheduler::Scheduler()
{
    slots_ = new TimerSlot[kMaxTimers];
    for (std::size_t i = 0; i < kMaxTimers; ++i) {
        slots_[i].owner = this;
    }
    lock_ = xSemaphoreCreateMutex();
}

Scheduler::~Scheduler()
{
    if (slots_ != nullptr) {
        for (std::size_t i = 0; i < kMaxTimers; ++i) {
            if (slots_[i].timer != nullptr) {
                esp_timer_stop(slots_[i].timer);
                esp_timer_delete(slots_[i].timer);
            }
        }
        delete[] slots_;
    }
    if (lock_ != nullptr) {
        vSemaphoreDelete(as_sem(lock_));
    }
}

Scheduler::Handle Scheduler::after_us(std::chrono::microseconds delay, const Event& event)
{
    return start_timer(delay, false, event);
}

Scheduler::Handle Scheduler::every_us(std::chrono::microseconds period, const Event& event)
{
    return start_timer(period, true, event);
}

Scheduler::Handle Scheduler::start_timer(std::chrono::microseconds interval, bool periodic,
                                         const Event& event)
{
    if (interval.count() <= 0) {
        ESP_LOGE(kTag, "timer interval must be positive");
        return {};
    }

    xSemaphoreTake(as_sem(lock_), portMAX_DELAY);
    TimerSlot* slot = find_free_slot();
    if (slot == nullptr) {
        xSemaphoreGive(as_sem(lock_));
        ESP_LOGE(kTag, "no free timer slots");
        return {};
    }

    if (slot->timer == nullptr) {
        const esp_timer_create_args_t args = {
            .callback = &Scheduler::timer_callback,
            .arg = slot,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "plat/tmr",
            .skip_unhandled_events = true,
        };
        const esp_err_t created = esp_timer_create(&args, &slot->timer);
        if (created != ESP_OK) {
            xSemaphoreGive(as_sem(lock_));
            ESP_LOGE(kTag, "esp_timer_create failed: %s", esp_err_to_name(created));
            return {};
        }
    } else {
        esp_timer_stop(slot->timer);
    }

    slot->in_use = true;
    slot->periodic = periodic;
    slot->id = next_id_++;
    if (next_id_ == 0) {
        next_id_ = 1;
    }
    slot->event = event;
    const Handle handle{slot->id};
    const uint64_t us = static_cast<uint64_t>(interval.count());
    xSemaphoreGive(as_sem(lock_));

    const esp_err_t started =
        periodic ? esp_timer_start_periodic(slot->timer, us) : esp_timer_start_once(slot->timer, us);
    if (started != ESP_OK) {
        xSemaphoreTake(as_sem(lock_), portMAX_DELAY);
        slot->in_use = false;
        slot->id = 0;
        xSemaphoreGive(as_sem(lock_));
        ESP_LOGE(kTag, "esp_timer_start failed: %s", esp_err_to_name(started));
        return {};
    }

    return handle;
}

void Scheduler::cancel(Handle handle)
{
    if (!handle) {
        return;
    }

    xSemaphoreTake(as_sem(lock_), portMAX_DELAY);
    TimerSlot* slot = find_slot(handle.id);
    if (slot == nullptr) {
        xSemaphoreGive(as_sem(lock_));
        return;
    }

    slot->in_use = false;
    slot->id = 0;
    esp_timer_handle_t timer = slot->timer;
    xSemaphoreGive(as_sem(lock_));

    if (timer != nullptr) {
        esp_timer_stop(timer);
    }
}

void Scheduler::timer_callback(void* arg)
{
    auto* slot = static_cast<TimerSlot*>(arg);
    slot->owner->on_timer(*slot);
}

void Scheduler::on_timer(TimerSlot& slot)
{
    Event event{};
    {
        xSemaphoreTake(as_sem(lock_), portMAX_DELAY);
        if (!slot.in_use) {
            xSemaphoreGive(as_sem(lock_));
            return;
        }
        event = slot.event;
        if (!slot.periodic) {
            slot.in_use = false;
            slot.id = 0;
        }
        xSemaphoreGive(as_sem(lock_));
    }

    (void)Runtime::instance().post_event(event);
}

Scheduler::TimerSlot* Scheduler::find_free_slot()
{
    for (std::size_t i = 0; i < kMaxTimers; ++i) {
        if (!slots_[i].in_use) {
            return &slots_[i];
        }
    }
    return nullptr;
}

Scheduler::TimerSlot* Scheduler::find_slot(uint32_t id)
{
    for (std::size_t i = 0; i < kMaxTimers; ++i) {
        if (slots_[i].in_use && slots_[i].id == id) {
            return &slots_[i];
        }
    }
    return nullptr;
}

}  // namespace platform
