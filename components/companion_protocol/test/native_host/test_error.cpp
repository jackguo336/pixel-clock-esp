#include <array>
#include <cstdint>
#include <span>
#include <variant>

#include "companion_protocol/message_decoder.hpp"
#include "companion_protocol/message_encoder.hpp"
#include "gtest/gtest.h"
#include "utils.hpp"

using companion_protocol_test::expect_same_bytes;
using companion_protocol_test::make_message;

namespace {

using companion_protocol::Error;
using companion_protocol::Message;
using companion_protocol::MessageType;
using companion_protocol::Status;
using companion_protocol::decode_message;
using companion_protocol::encode_message;
using companion_protocol::kErrorBodyBytes;
using companion_protocol::kHeaderBytes;
using companion_protocol::kMaxMessageBytes;

}  // namespace

TEST(Error, DocumentedExampleMatchesEncoder)
{
    // Example message from components/companion_protocol/PROTOCOL.md.
    const auto error_message = std::to_array<uint8_t>({
        0x01, 0xFF, 0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x02,
    });
    const Error error{
        .code = Status::InvalidField,
        .failed_message_type = MessageType::SetWifiCredentials,
    };
    std::array<uint8_t, error_message.size()> encoded_error{};
    ASSERT_EQ(encode_message(make_message(0, error), encoded_error), error_message.size());
    expect_same_bytes(encoded_error, error_message);

    Message decoded_error_message{};
    ASSERT_EQ(decode_message(error_message, decoded_error_message), Status::Ok);
    const Error* decoded_error = std::get_if<Error>(&decoded_error_message.body);
    ASSERT_NE(decoded_error, nullptr);
    EXPECT_EQ(decoded_error->code, Status::InvalidField);
    EXPECT_EQ(decoded_error->failed_message_type, MessageType::SetWifiCredentials);
}

TEST(Error, RejectsMalformedBody)
{
    Error error{
        .code = Status::StorageFailed,
        .failed_message_type = MessageType::Ack,
    };
    const std::array<uint8_t, 1> error_short{0x08};
    EXPECT_EQ(error.decode(error_short), Status::MalformedBody);
    const std::array<uint8_t, 3> error_trailing{0x08, 0x02, 0xFF};
    EXPECT_EQ(error.decode(error_trailing), Status::MalformedBody);
    EXPECT_EQ(error.code, Status::StorageFailed);
    EXPECT_EQ(error.failed_message_type, MessageType::Ack);
}
