#pragma once

// Private to companion_protocol sources. Message encoders share this header writer.

#include "byte_cursor.hpp"
#include "companion_protocol/frame.hpp"

namespace companion_protocol {

[[nodiscard]] inline bool write_frame_header(ByteWriter& writer, MessageType type, uint16_t request_id,
                                              uint16_t body_length)
{
    constexpr uint8_t kFlagsNone = 0;
    return writer.write_u8(kProtocolVersion) && writer.write_u8(static_cast<uint8_t>(type))
        && writer.write_u8(kFlagsNone) && writer.write_u16(request_id) && writer.write_u16(body_length);
}

}  // namespace companion_protocol
