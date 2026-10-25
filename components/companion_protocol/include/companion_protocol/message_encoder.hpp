#pragma once

#include <cstddef>
#include <span>

#include "companion_protocol/message.hpp"

namespace companion_protocol {

[[nodiscard]] std::size_t encode_message(const Message& message, std::span<uint8_t> out);

}  // namespace companion_protocol
