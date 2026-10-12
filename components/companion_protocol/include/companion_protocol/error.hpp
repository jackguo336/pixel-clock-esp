#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "companion_protocol/frame.hpp"

namespace companion_protocol {

inline constexpr std::size_t kErrorCodeBytes = 1;
inline constexpr std::size_t kFailedMessageTypeBytes = 1;
inline constexpr std::size_t kErrorBodyBytes = kErrorCodeBytes + kFailedMessageTypeBytes;
inline constexpr std::size_t kErrorFrameBytes = kHeaderBytes + kErrorBodyBytes;

static_assert(kErrorFrameBytes <= kMaxFrameBytes);

struct Error {
    Status code{};
    MessageType failed_message_type{};
};

// Writes an Error frame. Returns the frame length, or 0 if `out` is too small.
[[nodiscard]] std::size_t encode_error(uint16_t request_id, const Error& error, std::span<uint8_t> out);

// Copies the code and failed message type. `error` is left unchanged when the body is not two bytes.
[[nodiscard]] Status decode_error(std::span<const uint8_t> body, Error& error);

}  // namespace companion_protocol
