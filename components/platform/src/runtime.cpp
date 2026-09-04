#include "platform/runtime.hpp"

#include <inttypes.h>

#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace platform {
namespace {

constexpr char kTag[] = "runtime";
// Stack for the "ctrl" task, in bytes (ESP-IDF xTaskCreate units). Every
// component start/stop/on_event/on_command runs on this stack. 6144 is a
// starting budget (6 KiB), not a high-water-mark measurement; tune later with
// uxTaskGetStackHighWaterMark.
constexpr uint32_t kControlStackBytes = 6144;
constexpr UBaseType_t kControlPriority = 5;
constexpr uint32_t kReceiveTimeoutMs = 1000;
constexpr uint32_t kPostTimeoutMs = 0;

}  // namespace

Runtime& Runtime::instance()
{
    static Runtime runtime;
    return runtime;
}

void Runtime::add(RuntimeComponent& component)
{
    if (started_) {
        ESP_LOGE(kTag, "cannot add component after start");
        return;
    }
    if (component_count_ >= kMaxComponents) {
        ESP_LOGE(kTag, "component table full");
        return;
    }
    components_[component_count_++] = &component;
}

void Runtime::start()
{
    if (started_) {
        ESP_LOGW(kTag, "start ignored; already running");
        return;
    }
    started_ = true;

    TaskHandle_t handle = nullptr;
    const BaseType_t ok = xTaskCreate(&Runtime::control_task_thunk, "ctrl", kControlStackBytes, this,
                                      kControlPriority, &handle);
    if (ok != pdPASS) {
        started_ = false;
        ESP_LOGE(kTag, "failed to create control task");
        return;
    }
    control_task_ = handle;
}

void Runtime::stop()
{
    for (std::size_t i = component_count_; i > 0; --i) {
        components_[i - 1]->stop();
    }
}

bool Runtime::post_event(const Event& event)
{
    const bool ok = mailbox_.post(make_event_message(event), kPostTimeoutMs);
    if (!ok) {
        ESP_LOGW(kTag, "post_event failed type=%u", static_cast<unsigned>(event.type));
    }
    return ok;
}

bool Runtime::post_command(const Command& command)
{
    const bool ok = mailbox_.post(make_command_message(command), kPostTimeoutMs);
    if (!ok) {
        ESP_LOGW(kTag, "post_command failed type=%u target=%s", static_cast<unsigned>(command.type),
                 command.target != nullptr ? command.target->name() : "-");
    }
    return ok;
}

bool Runtime::post_event_from_isr(const Event& event, bool* yielded)
{
    return mailbox_.post_from_isr(make_event_message(event), yielded);
}

int64_t Runtime::now() const
{
    return esp_timer_get_time();
}

// FreeRTOS task entry is void(void*). start() passes the Runtime instance `this` as void* arg;
// cast it back to Runtime* and run the instance method that actually owns the loop.
void Runtime::control_task_thunk(void* arg)
{
    static_cast<Runtime*>(arg)->control_loop();
}

void Runtime::control_loop()
{
    const esp_err_t twdt = esp_task_wdt_add(xTaskGetCurrentTaskHandle());
    if (twdt != ESP_OK) {
        ESP_LOGW(kTag, "TWDT add failed: %s", esp_err_to_name(twdt));
    }

    ESP_LOGI(kTag, "control task starting %u component(s)", static_cast<unsigned>(component_count_));
    for (std::size_t i = 0; i < component_count_; ++i) {
        components_[i]->start();
    }

    for (;;) {
        log_mailbox_stats();
        Message message{};
        const bool has_message = mailbox_.receive(message, kReceiveTimeoutMs);
        // Tell watchdog that runtime is still alive so it does not reset the chip.
        (void)esp_task_wdt_reset();
        if (has_message) {
            dispatch(message);
        }
    }
}

void Runtime::dispatch(const Message& message)
{
    if (message.kind == MessageType::Event) {
        for (std::size_t i = 0; i < component_count_; ++i) {
            components_[i]->on_event(message.event);
        }
        return;
    }

    RuntimeComponent* target = find_component(message.command.target);
    if (target == nullptr) {
        ESP_LOGW(kTag, "command type=%u dropped; unknown target=%p",
                 static_cast<unsigned>(message.command.type),
                 static_cast<void*>(message.command.target));
        return;
    }
    target->on_command(message.command);
}

RuntimeComponent* Runtime::find_component(RuntimeComponent* component) const
{
    if (component == nullptr) {
        return nullptr;
    }
    for (std::size_t i = 0; i < component_count_; ++i) {
        if (components_[i] == component) {
            return components_[i];
        }
    }
    return nullptr;
}

void Runtime::log_mailbox_stats()
{
    const uint32_t drops = mailbox_.drop_count();
    const uint32_t overflows = mailbox_.overflow_count();
    if (drops == last_drop_count_ && overflows == last_overflow_count_) {
        return;
    }
    ESP_LOGW(kTag, "mailbox drops=%" PRIu32 " overflows=%" PRIu32 " queued=%u", drops, overflows,
             static_cast<unsigned>(mailbox_.size()));
    last_drop_count_ = drops;
    last_overflow_count_ = overflows;
}

}  // namespace platform
