#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "platform/message.hpp"

namespace platform {

class Scheduler {
public:
    static constexpr std::size_t kMaxTimers = 8;

    struct Handle {
        uint32_t id{0};

        explicit operator bool() const { return id != 0; }
    };

    Scheduler();
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    template <typename Rep, typename Period>
    Handle after(std::chrono::duration<Rep, Period> delay, const Event& event)
    {
        return after_us(std::chrono::duration_cast<std::chrono::microseconds>(delay), event);
    }

    template <typename Rep, typename Period>
    Handle every(std::chrono::duration<Rep, Period> period, const Event& event)
    {
        return every_us(std::chrono::duration_cast<std::chrono::microseconds>(period), event);
    }

    void cancel(Handle handle);

private:
    Handle after_us(std::chrono::microseconds delay, const Event& event);
    Handle every_us(std::chrono::microseconds period, const Event& event);
    Handle start_timer(std::chrono::microseconds interval, bool periodic, const Event& event);

    struct TimerSlot;

    static void timer_callback(void* arg);
    void on_timer(TimerSlot& slot);
    TimerSlot* find_free_slot();
    TimerSlot* find_slot(uint32_t id);

    TimerSlot* slots_{nullptr};
    void* lock_{nullptr};
    uint32_t next_id_{1};
};

}  // namespace platform
