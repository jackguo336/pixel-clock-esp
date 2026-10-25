#pragma once

#include <span>

#include "companion_protocol/message.hpp"

namespace companion_protocol {

[[nodiscard]] Status decode_header(std::span<const uint8_t> bytes, MessageHeader& header);

[[nodiscard]] Status decode_message(std::span<const uint8_t> bytes, Message& message);

}  // namespace companion_protocol
