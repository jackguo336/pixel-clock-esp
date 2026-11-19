#include "ble_link/outbound_message_splitter.hpp"

#include <algorithm>

#include "companion_protocol/message_header.hpp"

namespace ble_link {
namespace {

constexpr std::size_t kPayloadBytesAtMinimumMtu = kMinimumAttMtu - kNotifyHeaderBytes;

static_assert(kMinimumAttMtu > kNotifyHeaderBytes);
static_assert((companion_protocol::kMaxMessageBytes + kPayloadBytesAtMinimumMtu - 1) / kPayloadBytesAtMinimumMtu
              == kMaxNotifyPieces);

}  // namespace

bool message_split_piece_lengths(std::size_t message_length, uint16_t att_mtu,
                                 std::span<std::size_t> message_piece_lengths, std::size_t& message_piece_count)
{
    if (message_length == 0 || message_length > companion_protocol::kMaxMessageBytes || att_mtu < kMinimumAttMtu) {
        message_piece_count = 0;
        return false;
    }

    const std::size_t payload_bytes = static_cast<std::size_t>(att_mtu) - kNotifyHeaderBytes;
    const std::size_t piece_total = (message_length + payload_bytes - 1) / payload_bytes;
    if (message_piece_lengths.size() < piece_total) {
        message_piece_count = 0;
        return false;
    }

    std::size_t offset = 0;
    std::size_t index = 0;
    while (offset < message_length) {
        const std::size_t piece_length = std::min(payload_bytes, message_length - offset);
        message_piece_lengths[index] = piece_length;
        offset += piece_length;
        ++index;
    }

    message_piece_count = index;
    return true;
}

}  // namespace ble_link
