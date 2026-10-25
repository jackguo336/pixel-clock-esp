#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "companion_protocol/message_encoder.hpp"
#include "gtest/gtest.h"
#include "utils.hpp"

using companion_protocol_test::expect_unwritten;
using companion_protocol_test::kUnwrittenByte;
using companion_protocol_test::make_message;

namespace {

using companion_protocol::Ack;
using companion_protocol::DeviceInfo;
using companion_protocol::Error;
using companion_protocol::GetDeviceInfo;
using companion_protocol::Message;
using companion_protocol::MessageType;
using companion_protocol::SetWifiCredentials;
using companion_protocol::Status;
using companion_protocol::encode_message;
using companion_protocol::kBodyLengthOffset;
using companion_protocol::kHeaderBytes;
using companion_protocol::kMaxMessageBytes;
using companion_protocol::kMessageTypeOffset;

constexpr uint16_t kExampleRequestId = 0x1234;

template <typename Encode>
void expect_one_byte_short_writes_nothing(Encode&& encode)
{
    std::array<uint8_t, kMaxMessageBytes> full{};
    const std::size_t length = encode(std::span<uint8_t>(full));
    if (length == 0 || length > full.size()) {
        ADD_FAILURE() << "encoder did not produce a message";
        return;
    }

    std::array<uint8_t, kMaxMessageBytes + 1> storage{};
    storage.fill(kUnwrittenByte);
    const std::size_t short_length = length - 1;
    const std::size_t written = encode(std::span<uint8_t>(storage.data(), short_length));
    EXPECT_EQ(written, 0u);
    expect_unwritten(storage);
}

}  // namespace

TEST(MessageEncoder, DefaultMessageWritesNothing)
{
    std::array<uint8_t, kMaxMessageBytes> buffer{};
    buffer.fill(kUnwrittenByte);
    EXPECT_EQ(encode_message(Message{}, buffer), 0u);
    expect_unwritten(buffer);
}

TEST(MessageEncoder, UsesBodyMessageType)
{
    const Message message{
        .header = {
            .message_type = MessageType::Error,
            .request_id = kExampleRequestId,
        },
        .body = Ack{.status = Status::Ok},
    };
    std::array<uint8_t, kMaxMessageBytes> buffer{};
    buffer.fill(kUnwrittenByte);
    const std::size_t length = encode_message(message, buffer);
    ASSERT_EQ(length, kHeaderBytes + 1);
    EXPECT_EQ(buffer[kMessageTypeOffset], static_cast<uint8_t>(MessageType::Ack));
    EXPECT_EQ(buffer[kBodyLengthOffset], 1);
    EXPECT_EQ(buffer[kBodyLengthOffset + 1], 0);
    EXPECT_EQ(buffer[length], kUnwrittenByte);
}

TEST(MessageEncoder, RejectsShortBuffer)
{
    expect_one_byte_short_writes_nothing(
        [](std::span<uint8_t> out) { return encode_message(make_message(0x0001, GetDeviceInfo{}), out); });

    SetWifiCredentials credentials{};
    ASSERT_TRUE(credentials.ssid.assign(std::string_view{"home"}));
    ASSERT_TRUE(credentials.passphrase.assign(std::string_view{"password"}));
    expect_one_byte_short_writes_nothing(
        [&](std::span<uint8_t> out) { return encode_message(make_message(kExampleRequestId, credentials), out); });

    DeviceInfo info{};
    ASSERT_TRUE(info.firmware_version.assign(std::string_view{"1.0.0"}));
    expect_one_byte_short_writes_nothing(
        [&](std::span<uint8_t> out) { return encode_message(make_message(0x0001, info), out); });

    expect_one_byte_short_writes_nothing(
        [](std::span<uint8_t> out) { return encode_message(make_message(kExampleRequestId, Ack{}), out); });

    const Error error{
        .code = Status::InvalidField,
        .failed_message_type = MessageType::GetDeviceInfo,
    };
    expect_one_byte_short_writes_nothing(
        [&](std::span<uint8_t> out) { return encode_message(make_message(0, error), out); });
}
