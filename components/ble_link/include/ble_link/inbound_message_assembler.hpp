#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "companion_protocol/message_header.hpp"

namespace ble_link {

enum class AppendResult { NeedMore, Finished, Rejected };

/**
 * Joins inbound companion writes into one message.
 * Use on the Bluetooth host task, which is the only writer. One assembler
 * serves the one connection.
 */
class InboundMessageAssembler {
public:
    static constexpr std::size_t kMessageBytes = companion_protocol::kMaxMessageBytes;

    /// A write of this length is refused before append. drop_count stays unchanged.
    [[nodiscard]] static constexpr bool write_exceeds_message(std::size_t write_length)
    {
        return write_length > kMessageBytes;
    }

    /// Appends one piece, in order. The piece is at most one message.
    /// Finished: message holds the completed bytes and message_length is their count.
    /// NeedMore: message and message_length are unchanged.
    /// Rejected: a stored message was thrown away and drop_count increased by one.
    [[nodiscard]] AppendResult append(std::span<const uint8_t> piece, std::span<uint8_t> message,
                                      std::size_t& message_length);

    /// Clears a partial message that is already waiting. Does not change drop_count.
    void reset();

    [[nodiscard]] uint32_t drop_count() const;

private:
    std::array<uint8_t, kMessageBytes> held_{};
    std::size_t held_count_{0};
    uint32_t drop_count_{0};
};

}  // namespace ble_link
