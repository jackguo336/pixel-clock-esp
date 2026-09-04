#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

#include "platform/message_types.hpp"

namespace platform {

class RuntimeComponent;

inline constexpr std::size_t kPayloadCapacity = 32;

struct Event {
    EventType type{EventType::None};
    RuntimeComponent* source{nullptr};
    // Caller-owned correlation id. Copy from a command onto its reply so the
    // originator can match them. The platform does not set or read this.
    uint32_t generation{0};
    Priority priority{Priority::Normal};
    // 0: coalescing disabled, newer event does not overwrite older event in the mailbox.
    // !=0: coalescing enabled, newer event overwrites older event in the mailbox with 
    // the same coalesce key and event type.
    uint16_t coalesce_key{0};
    uint8_t payload_size{0};
    alignas(4) uint8_t payload[kPayloadCapacity]{};
};

struct Command {
    CommandType type{CommandType::None};
    RuntimeComponent* target{nullptr};
    RuntimeComponent* source{nullptr};
    // See Event::generation.
    uint32_t generation{0};
    uint8_t payload_size{0};
    alignas(4) uint8_t payload[kPayloadCapacity]{};
};

struct Message {
    MessageType kind{MessageType::Event};
    union {
        Event event;
        Command command;
    };
};

static_assert(std::is_trivially_copyable_v<Event>);
static_assert(std::is_trivially_copyable_v<Command>);
static_assert(std::is_trivially_copyable_v<Message>);

inline Message make_event_message(const Event& event)
{
    Message message{};
    message.kind = MessageType::Event;
    message.event = event;
    return message;
}

inline Message make_command_message(const Command& command)
{
    Message message{};
    message.kind = MessageType::Command;
    message.command = command;
    return message;
}

template <typename T>
inline void set_payload(Event& event, const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= kPayloadCapacity);
    std::memcpy(event.payload, &value, sizeof(T));
    event.payload_size = static_cast<uint8_t>(sizeof(T));
}

template <typename T>
inline void set_payload(Command& command, const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= kPayloadCapacity);
    std::memcpy(command.payload, &value, sizeof(T));
    command.payload_size = static_cast<uint8_t>(sizeof(T));
}

}  // namespace platform
