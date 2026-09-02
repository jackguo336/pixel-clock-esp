#pragma once

#include <cstdint>

namespace platform {

enum class EventType : uint16_t {
    None = 0,
    Tick = 1,
    TicksPaused = 2,
};

enum class CommandType : uint16_t {
    None = 0,
    PauseTicks = 1,
};

enum class MessageType : uint8_t {
    Event = 0,
    Command = 1,
};

enum class Priority : uint8_t {
    Normal = 0,
    Critical = 1,
};

}  // namespace platform
