#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "companion_protocol/message_header.hpp"

namespace companion_protocol {

// Keep PROTOCOL.md in sync with this message.
struct GetDeviceInfo {
    static constexpr MessageType kMessageType = MessageType::GetDeviceInfo;

    [[nodiscard]] std::optional<std::size_t> encoded_body_size() const;

    [[nodiscard]] bool encode(std::span<uint8_t> out) const;

    [[nodiscard]] Status decode(std::span<const uint8_t> bytes);
};

}  // namespace companion_protocol
