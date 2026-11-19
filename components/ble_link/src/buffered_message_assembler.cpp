#include "ble_link/buffered_message_assembler.hpp"

#include <algorithm>

#include "companion_protocol/message_decoder.hpp"

namespace ble_link {

AppendResult BufferedMessageAssembler::append(std::span<const uint8_t> message_piece, std::span<uint8_t> message,
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

    std::size_t required_message_size = 0;
    if (validate_buffered_message(required_message_size) == AppendResult::Rejected) {
        return AppendResult::Rejected;
    }
    if (buffered_message_size_ < required_message_size) {
        return AppendResult::NeedMore;
    }
    if (message.size() < required_message_size) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }

    std::copy_n(buffered_message_.begin(), required_message_size, message.begin());
    message_length = required_message_size;
    reset();
    return AppendResult::Finished;
}

AppendResult BufferedMessageAssembler::validate_buffered_message(std::size_t& required_message_size)
{
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

    required_message_size = companion_protocol::kHeaderBytes + header.body_length;
    if (buffered_message_size_ > required_message_size) {
        drop_count_ += 1;
        reset();
        return AppendResult::Rejected;
    }
    return AppendResult::Finished;
}

void BufferedMessageAssembler::reset()
{
    buffered_message_.fill(0);
    buffered_message_size_ = 0;
}

uint32_t BufferedMessageAssembler::drop_count() const
{
    return drop_count_;
}

}  // namespace ble_link
