#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace ble_link {

/// The packet size used as soon as the phone connects. Once the phone and the
/// pixel clock agree on a bigger size, later messages use the size they agreed
/// on.
inline constexpr uint16_t kMinimumAttMtu = 23;
inline constexpr std::size_t kNotifyHeaderBytes = 3;  // opcode and attribute handle

// Worst case is an ATT MTU of 23: ceil(128 / 20) = 7.
inline constexpr std::size_t kMaxNotifyPieces = 7;

/// Fills message_piece_lengths with how many bytes of the message fit in each
/// packet sent back to the phone. Each length is at most att_mtu minus 3.
[[nodiscard]] bool message_split_piece_lengths(std::size_t message_length, uint16_t att_mtu,
                                               std::span<std::size_t> message_piece_lengths,
                                               std::size_t& message_piece_count);

}  // namespace ble_link
