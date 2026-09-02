#pragma once

#include "platform/component.hpp"
#include "platform/scheduler.hpp"

namespace demo {

class TickSource : public platform::Component {
public:
    const char* name() const override;
    void start() override;
    void stop() override;
    void on_command(const platform::Command& command) override;

private:
    platform::Scheduler::Handle timer_{};
};

class TickSink : public platform::Component {
public:
    explicit TickSink(TickSource& tick_source);

    const char* name() const override;
    void on_event(const platform::Event& event) override;

private:
    static constexpr uint32_t kPauseAfterTicks = 5;
    TickSource& tick_source_;
    uint32_t tick_count_{0};
    bool pause_requested_{false};
};

}  // namespace demo
