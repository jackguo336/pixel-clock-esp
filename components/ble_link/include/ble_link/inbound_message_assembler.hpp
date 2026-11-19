#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "companion_protocol/message_header.hpp"

namespace ble_link {

enum class AppendResult { NeedMore, Finished, Rejected };

/**
 * A Bluetooth write holds at most the agreed packet size minus 3 bytes. A longer companion message arrives as pieces, and this class joins them into one message.
 */
class InboundMessageAssembler {
public:
    static constexpr std::size_t kMessageBytes = companion_protocol::kMaxMessageBytes;

    /// A write of this length is refused before append. drop_count stays unchanged.
    [[nodiscard]] static constexpr bool write_exceeds_message(std::size_t write_length)
    {
        return write_length > kMessageBytes;
    }

    /// Adds the next received piece to the message being built. A piece never holds more than one whole message.
    /// Finished means the pieces now form one complete message, copied into message.
    /// NeedMore means this piece was kept, and the phone still has more to send.
    /// Rejected means this piece did not fit the message already stored, so that stored message was thrown away and drop_count increased by one. message and message_length are unchanged.
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
