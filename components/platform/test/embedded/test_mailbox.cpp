#include "unity.h"

#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "platform/mailbox.hpp"
#include "platform/message.hpp"

namespace {

constexpr uint32_t kShortTimeoutMs = 200;
constexpr uint32_t kBlockTimeoutMs = 2000;
constexpr uint32_t kJoinTimeoutMs = 3000;
constexpr uint32_t kTaskStackBytes = 4096;
constexpr UBaseType_t kTaskPriority = 5;

constexpr platform::EventType kTestEvent1{static_cast<platform::EventType>(100)};
constexpr platform::EventType kTestEvent2{static_cast<platform::EventType>(101)};
constexpr platform::CommandType kTestCommand1{static_cast<platform::CommandType>(100)};

platform::Message make_event(uint32_t generation,
                            platform::Priority priority = platform::Priority::Normal,
                            uint16_t coalesce_key = 0,
                            platform::EventType type = kTestEvent1)
{
    platform::Event event{};
    event.type = type;
    event.generation = generation;
    event.priority = priority;
    event.coalesce_key = coalesce_key;
    return platform::make_event_message(event);
}

platform::Message make_command(uint32_t generation, platform::CommandType type = kTestCommand1)
{
    platform::Command command{};
    command.type = type;
    command.generation = generation;
    return platform::make_command_message(command);
}

void assert_event(const platform::Message& message, uint32_t generation,
                  platform::Priority priority = platform::Priority::Normal,
                  uint16_t coalesce_key = 0,
                  platform::EventType type = kTestEvent1)
{
    TEST_ASSERT_EQUAL(static_cast<int>(platform::MessageType::Event),
                      static_cast<int>(message.kind));
    TEST_ASSERT_EQUAL(static_cast<int>(type), static_cast<int>(message.event.type));
    TEST_ASSERT_EQUAL_UINT32(generation, message.event.generation);
    TEST_ASSERT_EQUAL(static_cast<int>(priority), static_cast<int>(message.event.priority));
    TEST_ASSERT_EQUAL_UINT16(coalesce_key, message.event.coalesce_key);
}

void assert_command(const platform::Message& message, uint32_t generation,
                    platform::CommandType type = kTestCommand1)
{
    TEST_ASSERT_EQUAL(static_cast<int>(platform::MessageType::Command),
                      static_cast<int>(message.kind));
    TEST_ASSERT_EQUAL(static_cast<int>(type), static_cast<int>(message.command.type));
    TEST_ASSERT_EQUAL_UINT32(generation, message.command.generation);
}

void fill_with_events(platform::Mailbox& mailbox, std::size_t count, uint32_t start = 0)
{
    for (std::size_t i = 0; i < count; ++i) {
        TEST_ASSERT_TRUE(mailbox.post(make_event(start + static_cast<uint32_t>(i)), 0));
    }
}

void fill_with_commands(platform::Mailbox& mailbox, std::size_t count, uint32_t start = 0)
{
    for (std::size_t i = 0; i < count; ++i) {
        TEST_ASSERT_TRUE(mailbox.post(make_command(start + static_cast<uint32_t>(i)), 0));
    }
}

struct BlockingPost {
    platform::Mailbox* mailbox{nullptr};
    platform::Message message{};
    uint32_t timeout_ms{kBlockTimeoutMs};
    bool result{false};
    SemaphoreHandle_t done{nullptr};

    BlockingPost() { done = xSemaphoreCreateBinary(); }
    ~BlockingPost()
    {
        if (done != nullptr) {
            vSemaphoreDelete(done);
        }
    }

    BlockingPost(const BlockingPost&) = delete;
    BlockingPost& operator=(const BlockingPost&) = delete;
};

void blocking_post_task(void* arg)
{
    auto* ctx = static_cast<BlockingPost*>(arg);
    ctx->result = ctx->mailbox->post(ctx->message, ctx->timeout_ms);
    xSemaphoreGive(ctx->done);
    vTaskDelete(nullptr);
}

void start_blocking_post(BlockingPost& ctx)
{
    TEST_ASSERT_NOT_NULL(ctx.done);
    TEST_ASSERT_NOT_NULL(ctx.mailbox);
    const BaseType_t ok =
        xTaskCreate(&blocking_post_task, "mb_post", kTaskStackBytes, &ctx, kTaskPriority, nullptr);
    TEST_ASSERT_EQUAL(pdPASS, ok);
}

bool wait_done(BlockingPost& ctx, uint32_t timeout_ms = kJoinTimeoutMs)
{
    return xSemaphoreTake(ctx.done, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

bool wait_until(bool (*ready)(const platform::Mailbox&), const platform::Mailbox& mailbox,
                uint32_t timeout_ms)
{
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (xTaskGetTickCount() < deadline) {
        if (ready(mailbox)) {
            return true;
        }
        vTaskDelay(1);
    }
    return ready(mailbox);
}

bool has_drop(const platform::Mailbox& mailbox)
{
    return mailbox.drop_count() > 0;
}

bool has_overflow(const platform::Mailbox& mailbox)
{
    return mailbox.overflow_count() > 0;
}

}  // namespace

TEST_CASE("empty mailbox has zero size and counters", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_EQUAL(0, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.drop_count());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.overflow_count());
}

TEST_CASE("post then receive is FIFO", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.post(make_event(1), 0));
    TEST_ASSERT_TRUE(mailbox.post(make_command(2), 0));
    TEST_ASSERT_TRUE(mailbox.post(make_event(3), 0));
    TEST_ASSERT_EQUAL(3, mailbox.size());

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 1);
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_command(out, 2);
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 3);
    TEST_ASSERT_EQUAL(0, mailbox.size());
}

TEST_CASE("receive times out on an empty mailbox", "[mailbox]")
{
    platform::Mailbox mailbox;
    platform::Message out{};
    TEST_ASSERT_FALSE(mailbox.receive(out, 0));
    TEST_ASSERT_FALSE(mailbox.receive(out, kShortTimeoutMs));
    TEST_ASSERT_EQUAL(0, mailbox.size());
}

TEST_CASE("payload is preserved through post and receive", "[mailbox]")
{
    platform::Mailbox mailbox;
    platform::Event event{};
    event.type = kTestEvent1;
    event.generation = 11;
    const uint32_t payload = 0xA5A5A5A5;
    platform::set_payload(event, payload);

    TEST_ASSERT_TRUE(mailbox.post(platform::make_event_message(event), 0));

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), out.event.payload_size);
    uint32_t got = 0;
    std::memcpy(&got, out.event.payload, sizeof(got));
    TEST_ASSERT_EQUAL_UINT32(payload, got);
}

TEST_CASE("matching events with a non-zero coalesce key replace in place", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.post(make_event(1, platform::Priority::Normal, 7), 0));
    TEST_ASSERT_TRUE(mailbox.post(make_event(2), 0));
    TEST_ASSERT_TRUE(mailbox.post(make_event(3, platform::Priority::Normal, 7), 0));
    TEST_ASSERT_EQUAL(2, mailbox.size());

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 3, platform::Priority::Normal, 7);
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 2);
}

TEST_CASE("coalesce updates payload of the existing event", "[mailbox]")
{
    platform::Mailbox mailbox;
    platform::Event first{};
    first.type = kTestEvent1;
    first.coalesce_key = 4;
    platform::set_payload(first, static_cast<uint32_t>(1));
    platform::Event second = first;
    platform::set_payload(second, static_cast<uint32_t>(99));

    TEST_ASSERT_TRUE(mailbox.post(platform::make_event_message(first), 0));
    TEST_ASSERT_TRUE(mailbox.post(platform::make_event_message(second), 0));
    TEST_ASSERT_EQUAL(1, mailbox.size());

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    uint32_t got = 0;
    std::memcpy(&got, out.event.payload, sizeof(got));
    TEST_ASSERT_EQUAL_UINT32(99, got);
}

TEST_CASE("zero coalesce key does not merge events", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.post(make_event(1), 0));
    TEST_ASSERT_TRUE(mailbox.post(make_event(2), 0));
    TEST_ASSERT_EQUAL(2, mailbox.size());
}

TEST_CASE("different event types with the same coalesce key do not merge", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.post(make_event(1, platform::Priority::Normal, 9, kTestEvent1), 0));
    TEST_ASSERT_TRUE(mailbox.post(make_event(2, platform::Priority::Normal, 9, kTestEvent2), 0));
    TEST_ASSERT_EQUAL(2, mailbox.size());
}

TEST_CASE("commands are not coalesced", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.post(make_command(1), 0));
    TEST_ASSERT_TRUE(mailbox.post(make_command(2), 0));
    TEST_ASSERT_EQUAL(2, mailbox.size());
}

TEST_CASE("coalesce still succeeds when only reserved slots remain", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.post(make_event(1, platform::Priority::Normal, 3), 0));
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity - 1, 10);
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity, mailbox.size());

    TEST_ASSERT_TRUE(mailbox.post(make_event(99, platform::Priority::Normal, 3), 0));
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.drop_count());

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 99, platform::Priority::Normal, 3);
}

TEST_CASE("coalesce still succeeds when the mailbox is full", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.post(make_event(1, platform::Priority::Critical, 5), 0));
    fill_with_commands(mailbox, platform::Mailbox::kCapacity - 1, 10);
    TEST_ASSERT_EQUAL(platform::Mailbox::kCapacity, mailbox.size());

    TEST_ASSERT_TRUE(mailbox.post(make_event(42, platform::Priority::Critical, 5), 0));
    TEST_ASSERT_EQUAL(platform::Mailbox::kCapacity, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.overflow_count());

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 42, platform::Priority::Critical, 5);
}

TEST_CASE("non-critical posts fill up to the normal capacity", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity);
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.drop_count());
}

TEST_CASE("non-critical post is dropped when only reserved slots remain", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity);

    TEST_ASSERT_FALSE(mailbox.post(make_event(100), 0));
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(1, mailbox.drop_count());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.overflow_count());
}

TEST_CASE("commands occupy reserved slots after normal capacity is full", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity);

    for (std::size_t i = 0; i < platform::Mailbox::kCriticalReserved; ++i) {
        TEST_ASSERT_TRUE(mailbox.post(make_command(static_cast<uint32_t>(i + 1)), 0));
    }
    TEST_ASSERT_EQUAL(platform::Mailbox::kCapacity, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.drop_count());
}

TEST_CASE("critical events occupy reserved slots after normal capacity is full", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity);

    TEST_ASSERT_TRUE(mailbox.post(make_event(1, platform::Priority::Critical), 0));
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity + 1, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.drop_count());
}

TEST_CASE("post overflows and increments both counters when full", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_commands(mailbox, platform::Mailbox::kCapacity);

    TEST_ASSERT_FALSE(mailbox.post(make_command(99), 0));
    TEST_ASSERT_EQUAL(platform::Mailbox::kCapacity, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(1, mailbox.drop_count());
    TEST_ASSERT_EQUAL_UINT32(1, mailbox.overflow_count());

    TEST_ASSERT_FALSE(mailbox.post(make_event(100, platform::Priority::Critical), 0));
    TEST_ASSERT_EQUAL_UINT32(2, mailbox.drop_count());
    TEST_ASSERT_EQUAL_UINT32(2, mailbox.overflow_count());
}

TEST_CASE("ring wrap preserves FIFO order", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity, 0);

    platform::Message out{};
    for (uint32_t i = 0; i < 20; ++i) {
        TEST_ASSERT_TRUE(mailbox.receive(out, 0));
        assert_event(out, i);
    }

    fill_with_events(mailbox, 20, 100);
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity, mailbox.size());

    for (uint32_t i = 20; i < platform::Mailbox::kNormalCapacity; ++i) {
        TEST_ASSERT_TRUE(mailbox.receive(out, 0));
        assert_event(out, i);
    }
    for (uint32_t i = 0; i < 20; ++i) {
        TEST_ASSERT_TRUE(mailbox.receive(out, 0));
        assert_event(out, 100 + i);
    }
    TEST_ASSERT_EQUAL(0, mailbox.size());
}

TEST_CASE("post_from_isr inserts and can be received from a task", "[mailbox]")
{
    platform::Mailbox mailbox;
    bool yielded = true;
    TEST_ASSERT_TRUE(mailbox.post_from_isr(make_event(8), &yielded));
    TEST_ASSERT_FALSE(yielded);
    TEST_ASSERT_EQUAL(1, mailbox.size());

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 8);
}

TEST_CASE("post_from_isr coalesces and accepts a null yielded pointer", "[mailbox]")
{
    platform::Mailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.post_from_isr(make_event(1, platform::Priority::Normal, 2), nullptr));
    TEST_ASSERT_TRUE(mailbox.post_from_isr(make_event(9, platform::Priority::Normal, 2), nullptr));
    TEST_ASSERT_EQUAL(1, mailbox.size());

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 9, platform::Priority::Normal, 2);
}

TEST_CASE("post_from_isr drops non-critical messages when only reserved slots remain", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity);

    bool yielded = true;
    TEST_ASSERT_FALSE(mailbox.post_from_isr(make_event(50), &yielded));
    TEST_ASSERT_FALSE(yielded);
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity, mailbox.size());
    TEST_ASSERT_EQUAL_UINT32(1, mailbox.drop_count());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.overflow_count());

    TEST_ASSERT_TRUE(mailbox.post_from_isr(make_event(51, platform::Priority::Critical), &yielded));
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity + 1, mailbox.size());
}

TEST_CASE("post waits for a normal slot after a drop", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity);

    BlockingPost poster;
    poster.mailbox = &mailbox;
    poster.message = make_event(500);
    start_blocking_post(poster);
    TEST_ASSERT_TRUE(wait_until(&has_drop, mailbox, 1000));

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 0);
    TEST_ASSERT_TRUE(wait_done(poster));
    TEST_ASSERT_TRUE(poster.result);
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity, mailbox.size());

    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_event(out, 1);
}

TEST_CASE("post waits for a reserved slot after overflow", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_commands(mailbox, platform::Mailbox::kCapacity);

    BlockingPost poster;
    poster.mailbox = &mailbox;
    poster.message = make_command(900);
    start_blocking_post(poster);
    TEST_ASSERT_TRUE(wait_until(&has_overflow, mailbox, 1000));

    platform::Message out{};
    TEST_ASSERT_TRUE(mailbox.receive(out, 0));
    assert_command(out, 0);
    TEST_ASSERT_TRUE(wait_done(poster));
    TEST_ASSERT_TRUE(poster.result);
    TEST_ASSERT_EQUAL(platform::Mailbox::kCapacity, mailbox.size());
}

TEST_CASE("post timeout expires when no slot is freed", "[mailbox]")
{
    platform::Mailbox mailbox;
    fill_with_events(mailbox, platform::Mailbox::kNormalCapacity);

    TEST_ASSERT_FALSE(mailbox.post(make_event(7), kShortTimeoutMs));
    TEST_ASSERT_EQUAL(platform::Mailbox::kNormalCapacity, mailbox.size());
    TEST_ASSERT_GREATER_THAN(0, mailbox.drop_count());
    TEST_ASSERT_EQUAL_UINT32(0, mailbox.overflow_count());
}
