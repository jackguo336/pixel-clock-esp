#include "ble_link/inbound_message_assembler.hpp"

#include <algorithm>

#include "companion_protocol/message_decoder.hpp"

namespace ble_link {

AppendResult InboundMessageAssembler::append(std::span<const uint8_t> piece, std::span<uint8_t> message,
                                              std::size_t& message_length)
{
    if (piece.empty()) {
        return AppendResult::NeedMore;
    }

    if (piece.size() > kMessageBytes - held_count_) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }

    std::copy_n(piece.begin(), piece.size(), held_.begin() + held_count_);
    held_count_ += piece.size();

    if (held_count_ < companion_protocol::kHeaderBytes) {
        return AppendResult::NeedMore;
    }

    companion_protocol::MessageHeader header{};
    const companion_protocol::Status header_status = companion_protocol::decode_header(
        std::span<const uint8_t>(held_.data(), companion_protocol::kHeaderBytes), header);
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
    if (held_count_ > needed) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }
    if (held_count_ < needed) {
        return AppendResult::NeedMore;
    }
    if (message.size() < needed) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }

    std::copy_n(held_.begin(), needed, message.begin());
    message_length = needed;
    reset();
    return AppendResult::Finished;
}

void InboundMessageAssembler::reset()
{
    held_.fill(0);
    held_count_ = 0;
}

uint32_t InboundMessageAssembler::drop_count() const
{
    return drop_count_;
}

}  // namespace ble_link
