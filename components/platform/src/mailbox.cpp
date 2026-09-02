#include "platform/mailbox.hpp"

#include <inttypes.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "freertos_handles.hpp"

namespace platform {
namespace {

constexpr char kTag[] = "mailbox";

}  // namespace

Mailbox::Mailbox()
{
    items_available_ = xSemaphoreCreateCounting(kCapacity, 0);
    normal_slots_available_ = xSemaphoreCreateCounting(kNormalCapacity, kNormalCapacity);
    high_priority_slots_available_ = xSemaphoreCreateCounting(kCriticalReserved, kCriticalReserved);
    lock_ = new SpinLock();
}

Mailbox::~Mailbox()
{
    if (items_available_ != nullptr) {
        vSemaphoreDelete(as_sem(items_available_));
    }
    if (normal_slots_available_ != nullptr) {
        vSemaphoreDelete(as_sem(normal_slots_available_));
    }
    if (high_priority_slots_available_ != nullptr) {
        vSemaphoreDelete(as_sem(high_priority_slots_available_));
    }
    delete as_lock(lock_);
}

bool Mailbox::uses_reserved_slots(const Message& message)
{
    if (message.kind == MessageType::Command) {
        return true;
    }
    return message.event.priority == Priority::Critical;
}

bool Mailbox::try_coalesce_locked(const Message& message)
{
    if (message.kind != MessageType::Event || message.event.coalesce_key == 0) {
        return false;
    }

    for (std::size_t i = 0; i < count_; ++i) {
        const std::size_t index = (head_ + i) % kCapacity;
        Message& existing = buffer_[index];
        if (existing.kind == MessageType::Event && existing.event.type == message.event.type &&
            existing.event.coalesce_key == message.event.coalesce_key) {
            existing = message;
            return true;
        }
    }
    return false;
}

Mailbox::InsertResult Mailbox::try_insert_locked(const Message& message)
{
    if (try_coalesce_locked(message)) {
        return InsertResult::Coalesced;
    }

    if (count_ >= kCapacity) {
        ++overflow_count_;
        ++drop_count_;
        return InsertResult::Overflow;
    }

    const std::size_t free = kCapacity - count_;
    if (!uses_reserved_slots(message) && free <= kCriticalReserved) {
        ++drop_count_;
        return InsertResult::Dropped;
    }

    buffer_[tail_] = message;
    tail_ = (tail_ + 1) % kCapacity;
    ++count_;
    if (count_ <= kNormalCapacity) {
        return InsertResult::InsertedNormal;
    }
    return InsertResult::InsertedHighPriority;
}

bool Mailbox::post(const Message& message, uint32_t timeout_ms)
{
    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    for (;;) {
        taskENTER_CRITICAL(&as_lock(lock_)->mux);
        const InsertResult result = try_insert_locked(message);
        taskEXIT_CRITICAL(&as_lock(lock_)->mux);

        if (result == InsertResult::InsertedNormal || result == InsertResult::InsertedHighPriority) {
            void* const slots_to_claim = (result == InsertResult::InsertedNormal)
                                    ? normal_slots_available_
                                    : high_priority_slots_available_;
            (void)xSemaphoreTake(as_sem(slots_to_claim), 0);
            xSemaphoreGive(as_sem(items_available_));
            return true;
        }
        if (result == InsertResult::Coalesced) {
            return true;
        }

        if (result == InsertResult::Overflow) {
            ESP_LOGW(kTag, "overflow; drops=%" PRIu32 " overflows=%" PRIu32, drop_count_,
                     overflow_count_);
        } else {
            ESP_LOGW(kTag, "dropped non-critical message; drops=%" PRIu32, drop_count_);
        }

        if (timeout_ms == 0) {
            return false;
        }

        const TickType_t elapsed = xTaskGetTickCount() - start;
        if (elapsed >= timeout_ticks) {
            return false;
        }

        void* const slots_to_wait_on =
            (result == InsertResult::Dropped || !uses_reserved_slots(message))
                ? normal_slots_available_
                : high_priority_slots_available_;
        if (xSemaphoreTake(as_sem(slots_to_wait_on), timeout_ticks - elapsed) != pdTRUE) {
            return false;
        }
    }
}

bool Mailbox::post_from_isr(const Message& message, bool* yielded)
{
    BaseType_t higher_priority_task_was_woken = pdFALSE;

    taskENTER_CRITICAL_ISR(&as_lock(lock_)->mux);
    const InsertResult result = try_insert_locked(message);
    taskEXIT_CRITICAL_ISR(&as_lock(lock_)->mux);

    if (result == InsertResult::InsertedNormal || result == InsertResult::InsertedHighPriority) {
        void* const slots_to_claim = (result == InsertResult::InsertedNormal)
                                ? normal_slots_available_
                                : high_priority_slots_available_;
        (void)xSemaphoreTakeFromISR(as_sem(slots_to_claim), &higher_priority_task_was_woken);
        xSemaphoreGiveFromISR(as_sem(items_available_), &higher_priority_task_was_woken);
    }

    if (yielded != nullptr) {
        *yielded = (higher_priority_task_was_woken == pdTRUE);
    }

    return result == InsertResult::InsertedNormal || result == InsertResult::InsertedHighPriority ||
           result == InsertResult::Coalesced;
}

bool Mailbox::receive(Message& out, uint32_t timeout_ms)
{
    if (xSemaphoreTake(as_sem(items_available_), pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        return false;
    }

    taskENTER_CRITICAL(&as_lock(lock_)->mux);
    out = buffer_[head_];
    head_ = (head_ + 1) % kCapacity;
    --count_;
    const std::size_t queued_count_after_receive = count_;
    taskEXIT_CRITICAL(&as_lock(lock_)->mux);

    if (queued_count_after_receive < kNormalCapacity) {
        xSemaphoreGive(as_sem(normal_slots_available_));
    } else {
        xSemaphoreGive(as_sem(high_priority_slots_available_));
    }
    return true;
}

std::size_t Mailbox::size() const
{
    return count_;
}

uint32_t Mailbox::drop_count() const
{
    return drop_count_;
}

uint32_t Mailbox::overflow_count() const
{
    return overflow_count_;
}

}  // namespace platform
