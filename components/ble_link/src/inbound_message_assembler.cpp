#include "ble_link/inbound_message_assembler.hpp"

#include <algorithm>

#include "companion_protocol/message_decoder.hpp"

namespace ble_link {

AppendResult InboundMessageAssembler::append(std::span<const uint8_t> message_piece, std::span<uint8_t> message,
                                              std::size_t& message_length)
{
    if (message_piece.empty()) {
        return AppendResult::NeedMore;
    }

    if (message_piece.size() > kMessageBytes - buffered_message_size_) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }

    std::copy_n(message_piece.begin(), message_piece.size(), buffered_message_.begin() + buffered_message_size_);
    buffered_message_size_ += message_piece.size();

    if (buffered_message_size_ < companion_protocol::kHeaderBytes) {
        return AppendResult::NeedMore;
    }

    companion_protocol::MessageHeader header{};
    const companion_protocol::Status header_status = companion_protocol::decode_header(
        std::span<const uint8_t>(buffered_message_.data(), companion_protocol::kHeaderBytes), header);
    if (header_status != companion_protocol::Status::Ok) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }

    constexpr std::size_t kMaxBodyBytes = kMessageBytes - companion_protocol::kHeaderBytes;
    if (header.body_length > kMaxBodyBytes) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }

    const std::size_t needed = companion_protocol::kHeaderBytes + header.body_length;
    if (buffered_message_size_ > needed) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }
    if (buffered_message_size_ < needed) {
        return AppendResult::NeedMore;
    }
    if (message.size() < needed) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }

    std::copy_n(buffered_message_.begin(), needed, message.begin());
    message_length = needed;
    reset();
    return AppendResult::Finished;
}

void InboundMessageAssembler::reset()
{
    buffered_message_.fill(0);
    buffered_message_size_ = 0;
}

uint32_t InboundMessageAssembler::drop_count() const
{
    return drop_count_;
}

}  // namespace ble_link
