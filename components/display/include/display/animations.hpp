#pragma once

#include <cstdint>
#include <span>
#include <variant>

#include "display/common.hpp"

namespace display {

enum class RepeatMode : uint8_t {
    Once = 0,
    Loop = 1,
};

enum class TransitionKind : uint8_t {
    None = 0,
    Fade = 1,
    ScrollUp = 2,
};

struct AnimationTiming {
    int64_t delay_us{0};
    int64_t interval_us{0};
    int64_t transition_duration_us{0};
    RepeatMode repeat{RepeatMode::Once};
};

// Blink the element's current paint. Cadence comes from AnimationTiming.
struct FlashAnimation {};

struct SequenceAnimation {
    // Frame IDs in a future scene's static storage. Each frame may use any
    // element payload; resolution and validation are downstream work.
    std::span<const ElementId> frames{};
    TransitionKind transition{TransitionKind::None};
};

struct PositionAnimation {
    Position from{};
    Position to{};
};

struct OpacityAnimation {
    uint8_t from{0};
    uint8_t to{0};
};

using AnimationPayload =
    std::variant<FlashAnimation, SequenceAnimation, PositionAnimation, OpacityAnimation>;

struct Animation {
    AnimationTiming timing{};
    AnimationPayload payload{};
};

}  // namespace display
