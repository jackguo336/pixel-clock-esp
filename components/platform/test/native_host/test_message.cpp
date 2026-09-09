#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "gtest/gtest.h"
#include "platform/message.hpp"

namespace {

struct SamplePayload {
    uint32_t count;
    uint16_t flags;
};

}  // namespace

TEST(Message, MakeEventMessageCopiesEventFields)
{
    platform::Event event{};
    event.type = platform::EventType::Tick;
    event.generation = 42;
    event.priority = platform::Priority::Critical;
    event.coalesce_key = 7;

    const platform::Message message = platform::make_event_message(event);

    EXPECT_EQ(message.kind, platform::MessageType::Event);
    EXPECT_EQ(message.event.type, platform::EventType::Tick);
    EXPECT_EQ(message.event.generation, 42u);
    EXPECT_EQ(message.event.priority, platform::Priority::Critical);
    EXPECT_EQ(message.event.coalesce_key, static_cast<uint16_t>(7));
    EXPECT_EQ(message.event.payload_size, 0);
}

TEST(Message, MakeCommandMessageCopiesCommandFields)
{
    platform::Command command{};
    command.type = platform::CommandType::PauseTicks;
    command.generation = 9;
    command.payload_size = 0;

    const platform::Message message = platform::make_command_message(command);

    EXPECT_EQ(message.kind, platform::MessageType::Command);
    EXPECT_EQ(message.command.type, platform::CommandType::PauseTicks);
    EXPECT_EQ(message.command.generation, 9u);
    EXPECT_EQ(message.command.payload_size, 0);
}

TEST(Message, SetPayloadCopiesValueIntoEvent)
{
    platform::Event event{};
    const SamplePayload payload{0x12345678u, 0xABCDu};

    platform::set_payload(event, payload);

    EXPECT_EQ(event.payload_size, sizeof(SamplePayload));

    SamplePayload copied{};
    std::memcpy(&copied, event.payload, sizeof(SamplePayload));
    EXPECT_EQ(copied.count, payload.count);
    EXPECT_EQ(copied.flags, payload.flags);
}

TEST(Message, SetPayloadCopiesValueIntoCommand)
{
    platform::Command command{};
    const SamplePayload payload{11u, 22};

    platform::set_payload(command, payload);

    EXPECT_EQ(command.payload_size, sizeof(SamplePayload));

    SamplePayload copied{};
    std::memcpy(&copied, command.payload, sizeof(SamplePayload));
    EXPECT_EQ(copied.count, payload.count);
    EXPECT_EQ(copied.flags, payload.flags);
}

TEST(Message, SetPayloadAcceptsFullCapacityBuffer)
{
    platform::Event event{};
    std::array<uint8_t, platform::kPayloadCapacity> payload{};
    payload.front() = 0x11;
    payload.back() = 0x22;

    platform::set_payload(event, payload);

    EXPECT_EQ(event.payload_size, platform::kPayloadCapacity);
    EXPECT_EQ(event.payload[0], 0x11);
    EXPECT_EQ(event.payload[platform::kPayloadCapacity - 1], 0x22);
}

TEST(Message, TypesAreTriviallyCopyable)
{
    static_assert(std::is_trivially_copyable_v<platform::Event>);
    static_assert(std::is_trivially_copyable_v<platform::Command>);
    static_assert(std::is_trivially_copyable_v<platform::Message>);

    platform::Event event{};
    event.type = platform::EventType::TicksPaused;
    event.generation = 3;
    const platform::Message original = platform::make_event_message(event);
    const platform::Message copy = original;

    EXPECT_EQ(copy.kind, original.kind);
    EXPECT_EQ(copy.event.type, original.event.type);
    EXPECT_EQ(copy.event.generation, original.event.generation);
}
