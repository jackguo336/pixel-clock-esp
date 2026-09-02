#include "unity.h"

#include <chrono>

#include "mock_esp_timer.hpp"
#include "platform/message.hpp"
#include "platform/runtime.hpp"
#include "platform/scheduler.hpp"

namespace {

using namespace std::chrono_literals;

constexpr platform::EventType kTestEvent1{static_cast<platform::EventType>(100)};

platform::Event make_event(uint32_t generation)
{
    platform::Event event{};
    event.type = kTestEvent1;
    event.generation = generation;
    return event;
}

void drain_runtime_mailbox()
{
    platform::Message message{};
    while (platform::Runtime::instance().mailbox().receive(message, 0)) {
    }
}

void assert_event(const platform::Event& event, uint32_t generation)
{
    TEST_ASSERT_EQUAL(static_cast<int>(kTestEvent1), static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_UINT32(generation, event.generation);
}

bool receive_event(uint32_t generation)
{
    platform::Message message{};
    if (!platform::Runtime::instance().mailbox().receive(message, 0)) {
        return false;
    }
    if (message.kind != platform::MessageType::Event) {
        return false;
    }
    assert_event(message.event, generation);
    return true;
}

void assert_one_shot_timer(std::size_t index, uint64_t interval_us)
{
    const mock_esp_timer::Timer* timer = mock_esp_timer::timer_at(index);
    TEST_ASSERT_NOT_NULL(timer);
    TEST_ASSERT_TRUE(timer->running);
    TEST_ASSERT_FALSE(timer->periodic);
    TEST_ASSERT_EQUAL(interval_us, timer->interval_us);
    TEST_ASSERT_NOT_NULL(timer->callback);
    TEST_ASSERT_EQUAL(static_cast<int>(ESP_TIMER_TASK), static_cast<int>(timer->dispatch_method));
    TEST_ASSERT_EQUAL_STRING("plat/tmr", timer->name);
    TEST_ASSERT_TRUE(timer->skip_unhandled_events);
}

}  // namespace

extern "C" void setUp(void)
{
    mock_esp_timer::reset();
}

extern "C" void tearDown(void)
{
    drain_runtime_mailbox();
    mock_esp_timer::restore();
}

TEST_CASE("non-positive delay does not create a timer", "[scheduler]")
{
    platform::Scheduler scheduler;
    TEST_ASSERT_FALSE(scheduler.after(0us, make_event(1)));
    TEST_ASSERT_FALSE(scheduler.after(std::chrono::microseconds{-1}, make_event(2)));
    TEST_ASSERT_EQUAL(0, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(0, mock_esp_timer::start_once_count());
}

TEST_CASE("after starts a one-shot timer with the delay in microseconds", "[scheduler]")
{
    platform::Scheduler scheduler;
    const platform::Scheduler::Handle handle = scheduler.after(250us, make_event(3));
    TEST_ASSERT_TRUE(handle);
    TEST_ASSERT_EQUAL(1, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(1, mock_esp_timer::start_once_count());
    TEST_ASSERT_EQUAL(0, mock_esp_timer::start_periodic_count());
    TEST_ASSERT_EQUAL(0, mock_esp_timer::stop_count());
    assert_one_shot_timer(0, 250);
}

TEST_CASE("every starts a periodic timer with the period in microseconds", "[scheduler]")
{
    platform::Scheduler scheduler;
    const platform::Scheduler::Handle handle = scheduler.every(40ms, make_event(4));
    TEST_ASSERT_TRUE(handle);
    TEST_ASSERT_EQUAL(1, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(0, mock_esp_timer::start_once_count());
    TEST_ASSERT_EQUAL(1, mock_esp_timer::start_periodic_count());

    const mock_esp_timer::Timer* timer = mock_esp_timer::timer_at(0);
    TEST_ASSERT_NOT_NULL(timer);
    TEST_ASSERT_TRUE(timer->running);
    TEST_ASSERT_TRUE(timer->periodic);
    TEST_ASSERT_EQUAL(40000, timer->interval_us);
}

TEST_CASE("after converts millisecond delays to microseconds", "[scheduler]")
{
    platform::Scheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.after(5ms, make_event(5)));
    assert_one_shot_timer(0, 5000);
}

TEST_CASE("one-shot fire posts the event and does not fire again", "[scheduler]")
{
    platform::Scheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.after(100us, make_event(7)));
    TEST_ASSERT_TRUE(mock_esp_timer::fire(0));
    TEST_ASSERT_TRUE(receive_event(7));
    TEST_ASSERT_FALSE(mock_esp_timer::fire(0));

    platform::Message leftover{};
    TEST_ASSERT_FALSE(platform::Runtime::instance().mailbox().receive(leftover, 0));
}

TEST_CASE("periodic fire posts the event and stays armed", "[scheduler]")
{
    platform::Scheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.every(10ms, make_event(8)));
    TEST_ASSERT_TRUE(mock_esp_timer::fire(0));
    TEST_ASSERT_TRUE(mock_esp_timer::fire(0));
    TEST_ASSERT_TRUE(receive_event(8));
    TEST_ASSERT_TRUE(receive_event(8));
}

TEST_CASE("one-shot slot is reused without creating another timer", "[scheduler]")
{
    platform::Scheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.after(100us, make_event(1)));
    TEST_ASSERT_TRUE(mock_esp_timer::fire(0));
    TEST_ASSERT_TRUE(receive_event(1));

    TEST_ASSERT_TRUE(scheduler.after(200us, make_event(2)));
    TEST_ASSERT_EQUAL(1, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(1, mock_esp_timer::stop_count());
    TEST_ASSERT_EQUAL(2, mock_esp_timer::start_once_count());
    assert_one_shot_timer(0, 200);

    TEST_ASSERT_TRUE(mock_esp_timer::fire(0));
    TEST_ASSERT_TRUE(receive_event(2));
}

TEST_CASE("cancel stops a running timer", "[scheduler]")
{
    platform::Scheduler scheduler;
    const platform::Scheduler::Handle handle = scheduler.every(1ms, make_event(9));
    TEST_ASSERT_TRUE(handle);
    scheduler.cancel(handle);

    TEST_ASSERT_EQUAL(1, mock_esp_timer::stop_count());
    const mock_esp_timer::Timer* timer = mock_esp_timer::timer_at(0);
    TEST_ASSERT_NOT_NULL(timer);
    TEST_ASSERT_FALSE(timer->running);
    TEST_ASSERT_FALSE(mock_esp_timer::fire(0));
}

TEST_CASE("cancel of an empty handle is a no-op", "[scheduler]")
{
    platform::Scheduler scheduler;
    scheduler.cancel({});
    TEST_ASSERT_EQUAL(0, mock_esp_timer::stop_count());
}

TEST_CASE("cancel of an unknown id is a no-op", "[scheduler]")
{
    platform::Scheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.after(1ms, make_event(1)));
    platform::Scheduler::Handle unknown;
    unknown.id = 99;
    scheduler.cancel(unknown);
    TEST_ASSERT_EQUAL(0, mock_esp_timer::stop_count());
    TEST_ASSERT_TRUE(mock_esp_timer::timer_at(0)->running);
}

TEST_CASE("cancelled timer callback does not post an event", "[scheduler]")
{
    platform::Scheduler scheduler;
    const platform::Scheduler::Handle handle = scheduler.after(1ms, make_event(11));
    scheduler.cancel(handle);
    TEST_ASSERT_TRUE(mock_esp_timer::invoke(0));

    platform::Message leftover{};
    TEST_ASSERT_FALSE(platform::Runtime::instance().mailbox().receive(leftover, 0));
}

TEST_CASE("concurrent timers get distinct handles", "[scheduler]")
{
    platform::Scheduler scheduler;
    const platform::Scheduler::Handle first = scheduler.after(1ms, make_event(1));
    const platform::Scheduler::Handle second = scheduler.after(2ms, make_event(2));
    TEST_ASSERT_TRUE(first);
    TEST_ASSERT_TRUE(second);
    TEST_ASSERT_NOT_EQUAL(first.id, second.id);
    TEST_ASSERT_EQUAL(2, mock_esp_timer::allocated_count());
}

TEST_CASE("no free slots returns an invalid handle", "[scheduler]")
{
    platform::Scheduler scheduler;
    platform::Scheduler::Handle handles[platform::Scheduler::kMaxTimers]{};
    for (std::size_t i = 0; i < platform::Scheduler::kMaxTimers; ++i) {
        handles[i] = scheduler.after(1ms, make_event(static_cast<uint32_t>(i + 1)));
        TEST_ASSERT_TRUE(handles[i]);
    }
    TEST_ASSERT_FALSE(scheduler.after(1ms, make_event(99)));
    TEST_ASSERT_EQUAL(platform::Scheduler::kMaxTimers, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(platform::Scheduler::kMaxTimers, mock_esp_timer::start_once_count());
}

TEST_CASE("create failure returns an invalid handle", "[scheduler]")
{
    mock_esp_timer::set_next_create_result(ESP_ERR_NO_MEM);
    platform::Scheduler scheduler;
    TEST_ASSERT_FALSE(scheduler.after(1ms, make_event(1)));
    TEST_ASSERT_EQUAL(1, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(0, mock_esp_timer::allocated_count());
    TEST_ASSERT_EQUAL(0, mock_esp_timer::start_once_count());
}

TEST_CASE("start failure returns an invalid handle and frees the slot", "[scheduler]")
{
    mock_esp_timer::set_next_start_result(ESP_ERR_INVALID_STATE);
    platform::Scheduler scheduler;
    TEST_ASSERT_FALSE(scheduler.after(1ms, make_event(1)));
    TEST_ASSERT_EQUAL(1, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(1, mock_esp_timer::start_once_count());
    TEST_ASSERT_EQUAL(1, mock_esp_timer::allocated_count());

    TEST_ASSERT_TRUE(scheduler.after(2ms, make_event(2)));
    TEST_ASSERT_EQUAL(1, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(1, mock_esp_timer::stop_count());
    TEST_ASSERT_EQUAL(2, mock_esp_timer::start_once_count());
    assert_one_shot_timer(0, 2000);
}

TEST_CASE("destructor stops and deletes created timers", "[scheduler]")
{
    {
        platform::Scheduler scheduler;
        TEST_ASSERT_TRUE(scheduler.after(1ms, make_event(1)));
        TEST_ASSERT_TRUE(scheduler.every(2ms, make_event(2)));
        TEST_ASSERT_EQUAL(0, mock_esp_timer::delete_count());
    }
    TEST_ASSERT_EQUAL(2, mock_esp_timer::stop_count());
    TEST_ASSERT_EQUAL(2, mock_esp_timer::delete_count());
    TEST_ASSERT_EQUAL(0, mock_esp_timer::allocated_count());
}

TEST_CASE("cancel then start reuses the existing timer", "[scheduler]")
{
    platform::Scheduler scheduler;
    const platform::Scheduler::Handle first = scheduler.after(1ms, make_event(1));
    scheduler.cancel(first);
    TEST_ASSERT_TRUE(scheduler.every(3ms, make_event(2)));
    TEST_ASSERT_EQUAL(1, mock_esp_timer::create_count());
    TEST_ASSERT_EQUAL(2, mock_esp_timer::stop_count());
    TEST_ASSERT_EQUAL(1, mock_esp_timer::start_once_count());
    TEST_ASSERT_EQUAL(1, mock_esp_timer::start_periodic_count());

    const mock_esp_timer::Timer* timer = mock_esp_timer::timer_at(0);
    TEST_ASSERT_NOT_NULL(timer);
    TEST_ASSERT_TRUE(timer->periodic);
    TEST_ASSERT_EQUAL(3000, timer->interval_us);
}
