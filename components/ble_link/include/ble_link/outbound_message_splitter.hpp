#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace ble_link {

/// The smallest packet a phone and the pixel clock can use. They may agree on a larger one.
inline constexpr uint16_t kMinimumAttMtu = 23;
inline constexpr std::size_t kNotifyHeaderBytes = 3;  // opcode and attribute handle

// Worst case is an ATT MTU of 23: ceil(128 / 20) = 7.
inline constexpr std::size_t kMaxNotifyPieces = 7;

/// Writes one length per notification, in order.
/// Returns false when the message is empty, longer than 128 bytes, or att_mtu
/// is below 23. On false, piece_count is 0 and piece_lengths is unchanged.
[[nodiscard]] bool message_split_piece_lengths(std::size_t message_length, uint16_t att_mtu,
                                               std::span<std::size_t> piece_lengths, std::size_t& piece_count);

}  // namespace ble_link
