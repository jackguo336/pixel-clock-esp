#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "companion_protocol/frame.hpp"

namespace companion_protocol {

struct Ack {
    Status status{Status::Ok};
};

// Writes an Ack frame. Returns the frame length, or 0 if `out` is too small.
[[nodiscard]] std::size_t encode_ack(uint16_t request_id, const Ack& ack, std::span<uint8_t> out);

// Copies the status byte. `ack` is left unchanged when the body is not one byte.
[[nodiscard]] Status decode_ack(std::span<const uint8_t> body, Ack& ack);

}  // namespace companion_protocol
