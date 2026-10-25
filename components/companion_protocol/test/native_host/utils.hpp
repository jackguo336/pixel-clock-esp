#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <variant>

#include "companion_protocol/message_decoder.hpp"
#include "gtest/gtest.h"

namespace companion_protocol_test {

using companion_protocol::Ack;
using companion_protocol::Message;
using companion_protocol::MessageBody;
using companion_protocol::MessageType;
using companion_protocol::Status;
using companion_protocol::decode_message;

inline constexpr uint8_t kUnwrittenByte = 0x5A;

inline void expect_same_bytes(std::span<const uint8_t> actual, std::span<const uint8_t> expected)
{
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(actual[index], expected[index]) << "at index " << index;
    }
}

inline void expect_unwritten(std::span<const uint8_t> bytes)
{
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if (bytes[index] != kUnwrittenByte) {
            EXPECT_EQ(bytes[index], kUnwrittenByte) << "at index " << index;
            return;
        }
    }
}

inline void expect_monostate(const Message& message)
{
    EXPECT_TRUE(std::holds_alternative<std::monostate>(message.body));
}

inline void expect_body_rejected(std::span<const uint8_t> bytes, Status expected, MessageType message_type,
                                 uint16_t request_id)
{
    Message message{
        .header = {.request_id = 1},
        .body = Ack{.status = Status::StorageFailed},
    };
    EXPECT_EQ(decode_message(bytes, message), expected);
    EXPECT_EQ(message.header.request_id, request_id);
    EXPECT_EQ(message.header.message_type, message_type);
    expect_monostate(message);
}

[[nodiscard]] inline Message make_message(uint16_t request_id, MessageBody body)
{
    return Message{
        .header = {.request_id = request_id},
        .body = std::move(body),
    };
}

}  // namespace companion_protocol_test
