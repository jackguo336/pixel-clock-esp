#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/mailbox.hpp"
#include "platform/runtime_component.hpp"
#include "platform/scheduler.hpp"

namespace platform {

class Runtime {
public:
    static constexpr std::size_t kMaxComponents = 16;

    static Runtime& instance();

    void add(RuntimeComponent& component);
    void start();
    void stop();

    bool post_event(const Event& event);
    bool post_command(const Command& command);
    bool post_event_from_isr(const Event& event, bool* yielded = nullptr);

    int64_t now() const;

    Scheduler& scheduler() { return scheduler_; }
    const Scheduler& scheduler() const { return scheduler_; }

    Mailbox& mailbox() { return mailbox_; }
    const Mailbox& mailbox() const { return mailbox_; }

private:
    Runtime() = default;

    static void control_task_thunk(void* arg);
    void control_loop();
    void dispatch(const Message& message);
    RuntimeComponent* find_component(RuntimeComponent* component) const;
    void log_mailbox_stats();

    RuntimeComponent* components_[kMaxComponents]{};
    std::size_t component_count_{0};
    Mailbox mailbox_{};
    Scheduler scheduler_{};
    void* control_task_{nullptr};
    bool started_{false};
    uint32_t last_drop_count_{0};
    uint32_t last_overflow_count_{0};
};

}  // namespace platform
