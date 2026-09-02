#include "demo_ticks.hpp"

#include <chrono>

#include "platform/log.hpp"
#include "platform/runtime.hpp"

namespace demo {
namespace {

constexpr auto kTickPeriod = std::chrono::milliseconds(500);

}  // namespace

const char* TickSource::name() const
{
    return "tick_src";
}

void TickSource::start()
{
    platform::Event tick{};
    tick.type = platform::EventType::Tick;
    tick.source = this;
    tick.priority = platform::Priority::Normal;
    timer_ = platform::Runtime::instance().scheduler().every(kTickPeriod, tick);
    PLATFORM_LOGI(this, "started periodic ticks");
}

void TickSource::stop()
{
    platform::Runtime::instance().scheduler().cancel(timer_);
    timer_ = {};
}

void TickSource::on_command(const platform::Command& command)
{
    if (command.type != platform::CommandType::PauseTicks) {
        return;
    }

    platform::Runtime::instance().scheduler().cancel(timer_);
    timer_ = {};

    platform::Event paused{};
    paused.type = platform::EventType::TicksPaused;
    paused.source = this;
    paused.generation = command.generation;
    paused.priority = platform::Priority::Critical;
    (void)platform::Runtime::instance().post_event(paused);
    PLATFORM_LOGI(this, "paused ticks");
}

TickSink::TickSink(TickSource& tick_source) : tick_source_(tick_source) {}

const char* TickSink::name() const
{
    return "tick_sink";
}

void TickSink::on_event(const platform::Event& event)
{
    if (event.type == platform::EventType::Tick) {
        ++tick_count_;
        PLATFORM_LOGI(this, "tick %u", static_cast<unsigned>(tick_count_));
        if (!pause_requested_ && tick_count_ >= kPauseAfterTicks) {
            pause_requested_ = true;
            platform::Command pause{};
            pause.type = platform::CommandType::PauseTicks;
            pause.target = &tick_source_;
            pause.source = this;
            (void)platform::Runtime::instance().post_command(pause);
            PLATFORM_LOGI(this, "requesting pause");
        }
        return;
    }

    if (event.type == platform::EventType::TicksPaused) {
        PLATFORM_LOGI(this, "ticks paused after %u", static_cast<unsigned>(tick_count_));
    }
}

}  // namespace demo
