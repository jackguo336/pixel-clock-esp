#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "companion_protocol/message_header.hpp"

namespace companion_protocol {

inline constexpr std::size_t kAckBodyBytes = 1;

// Keep PROTOCOL.md in sync with this message.
struct Ack {
    static constexpr MessageType kMessageType = MessageType::Ack;

    Status status{Status::Ok};

    [[nodiscard]] std::optional<std::size_t> encoded_body_size() const;

    [[nodiscard]] bool encode(std::span<uint8_t> out) const;

    [[nodiscard]] Status decode(std::span<const uint8_t> bytes);
};

}  // namespace companion_protocol
