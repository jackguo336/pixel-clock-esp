#include <array>
#include <climits>
#include <cstdint>
#include <span>
#include <variant>

#include "companion_protocol/message_decoder.hpp"
#include "gtest/gtest.h"
#include "utils.hpp"

using companion_protocol_test::expect_body_rejected;
using companion_protocol_test::expect_monostate;

namespace {

using companion_protocol::Ack;
using companion_protocol::GetDeviceInfo;
using companion_protocol::Message;
using companion_protocol::MessageHeader;
using companion_protocol::MessageType;
using companion_protocol::Status;
using companion_protocol::decode_header;
using companion_protocol::decode_message;
using companion_protocol::kBodyLengthOffset;
using companion_protocol::kFlagsNone;
using companion_protocol::kFlagsOffset;
using companion_protocol::kHeaderBytes;
using companion_protocol::kMaxMessageBytes;
using companion_protocol::kMessageTypeOffset;
using companion_protocol::kProtocolVersion;
using companion_protocol::kRequestIdOffset;
using companion_protocol::kVersionOffset;

constexpr uint16_t kExampleRequestId = 0x1234;
constexpr uint8_t kUnassignedMessageType = 0x42;
constexpr uint8_t kUnsupportedVersion = 2;
constexpr uint8_t kUnsupportedFlag = 1;
constexpr uint16_t kSentinelRequestId = 0xFFFF;
constexpr uint8_t kSentinelVersion = 9;
constexpr uint16_t kUnchangedRequestId = 7;
constexpr uint16_t kOtherUnchangedRequestId = 99;
constexpr uint16_t kExampleBodyLength = 0x000E;

}  // namespace

TEST(MessageDecoder, GetDeviceInfoHandBuiltMessageDecodes)
{
    const std::array<uint8_t, kHeaderBytes> bytes{
        0x01, 0x01, 0x00, 0x2A, 0x00, 0x00, 0x00,
    };

    Message message{};
    ASSERT_EQ(decode_message(bytes, message), Status::Ok);
    EXPECT_EQ(message.header.version, kProtocolVersion);
    EXPECT_EQ(message.header.message_type, MessageType::GetDeviceInfo);
    EXPECT_EQ(message.header.flags, kFlagsNone);
    EXPECT_EQ(message.header.request_id, 0x002Au);
    EXPECT_EQ(message.header.body_length, 0u);
    EXPECT_TRUE(std::holds_alternative<GetDeviceInfo>(message.body));
}

TEST(MessageDecoder, MultiByteHeaderFieldsAreLittleEndian)
{
    // request_id 0x1234 is the bytes 34 12. body_length 1 is the bytes 01 00.
    // The last byte is Ack status StorageFailed.
    const std::array<uint8_t, kHeaderBytes + 1> bytes{
        0x01, 0x80, 0x00, 0x34, 0x12, 0x01, 0x00, static_cast<uint8_t>(Status::StorageFailed),
    };

    Message message{};
    ASSERT_EQ(decode_message(bytes, message), Status::Ok);
    EXPECT_EQ(message.header.request_id, kExampleRequestId);
    EXPECT_EQ(message.header.body_length, 1u);
    const Ack* ack = std::get_if<Ack>(&message.body);
    ASSERT_NE(ack, nullptr);
    EXPECT_EQ(ack->status, Status::StorageFailed);
}

TEST(MessageDecoder, ShortInputReturnsMessageTooShort)
{
    const std::array<uint8_t, kHeaderBytes - 1> bytes{
        0x01, 0x01, 0x00, 0x34, 0x12, 0x00,
    };
    Message message{};
    message.header.request_id = kSentinelRequestId;
    message.header.message_type = MessageType::Ack;
    message.header.version = kSentinelVersion;
    message.body = Ack{.status = Status::StorageFailed};

    EXPECT_EQ(decode_message(bytes, message), Status::MessageTooShort);
    EXPECT_EQ(message.header.request_id, kSentinelRequestId);
    EXPECT_EQ(message.header.message_type, MessageType::Ack);
    EXPECT_EQ(message.header.version, kSentinelVersion);
    const Ack* ack = std::get_if<Ack>(&message.body);
    ASSERT_NE(ack, nullptr);
    EXPECT_EQ(ack->status, Status::StorageFailed);

    Message empty_message{};
    empty_message.header.request_id = kUnchangedRequestId;
    EXPECT_EQ(decode_message(std::span<const uint8_t>{}, empty_message), Status::MessageTooShort);
    EXPECT_EQ(empty_message.header.request_id, kUnchangedRequestId);
}

TEST(MessageDecoder, LongInputReturnsMessageTooLong)
{
    std::array<uint8_t, kMaxMessageBytes + 1> bytes{};
    bytes[kVersionOffset] = kProtocolVersion;
    bytes[kMessageTypeOffset] = static_cast<uint8_t>(MessageType::SetWifiCredentials);
    bytes[kFlagsOffset] = kFlagsNone;
    bytes[kRequestIdOffset] = static_cast<uint8_t>(kExampleRequestId);
    bytes[kRequestIdOffset + 1] = static_cast<uint8_t>(kExampleRequestId >> CHAR_BIT);

    Message message{};
    message.header.request_id = kSentinelRequestId;
    message.header.message_type = MessageType::Ack;
    message.header.version = kSentinelVersion;
    message.body = Ack{.status = Status::StorageFailed};
    EXPECT_EQ(decode_message(bytes, message), Status::MessageTooLong);
    EXPECT_EQ(message.header.request_id, kSentinelRequestId);
    EXPECT_EQ(message.header.message_type, MessageType::Ack);
    EXPECT_EQ(message.header.version, kSentinelVersion);
    const Ack* ack = std::get_if<Ack>(&message.body);
    ASSERT_NE(ack, nullptr);
    EXPECT_EQ(ack->status, Status::StorageFailed);
}

TEST(MessageDecoder, MaxSizedGetDeviceInfoBodyIsMalformed)
{
    std::array<uint8_t, kMaxMessageBytes> at_limit{};
    at_limit[kVersionOffset] = kProtocolVersion;
    at_limit[kMessageTypeOffset] = static_cast<uint8_t>(MessageType::GetDeviceInfo);
    at_limit[kFlagsOffset] = kFlagsNone;
    at_limit[kRequestIdOffset] = static_cast<uint8_t>(kExampleRequestId);
    at_limit[kRequestIdOffset + 1] = static_cast<uint8_t>(kExampleRequestId >> CHAR_BIT);
    const uint16_t body_length = static_cast<uint16_t>(kMaxMessageBytes - kHeaderBytes);
    at_limit[kBodyLengthOffset] = static_cast<uint8_t>(body_length);
    at_limit[kBodyLengthOffset + 1] = static_cast<uint8_t>(body_length >> CHAR_BIT);

    Message limit_message{};
    limit_message.body = Ack{.status = Status::StorageFailed};
    ASSERT_EQ(decode_message(at_limit, limit_message), Status::MalformedBody);
    EXPECT_EQ(limit_message.header.request_id, kExampleRequestId);
    EXPECT_EQ(limit_message.header.message_type, MessageType::GetDeviceInfo);
    expect_monostate(limit_message);
}

TEST(MessageDecoder, BodyLengthMismatchReportsRequestId)
{
    const std::array<uint8_t, kHeaderBytes> longer_than_bytes{
        0x01, 0x01, 0x00, 0x34, 0x12, 0x05, 0x00,
    };
    Message longer{};
    longer.body = Ack{.status = Status::StorageFailed};
    EXPECT_EQ(decode_message(longer_than_bytes, longer), Status::BodyLengthMismatch);
    EXPECT_EQ(longer.header.request_id, kExampleRequestId);
    EXPECT_EQ(longer.header.message_type, MessageType::GetDeviceInfo);
    expect_monostate(longer);

    const std::array<uint8_t, kHeaderBytes + 1> shorter_than_bytes{
        0x01, 0x02, 0x00, 0x34, 0x12, 0x00, 0x00, 0xFF,
    };
    Message shorter{};
    EXPECT_EQ(decode_message(shorter_than_bytes, shorter), Status::BodyLengthMismatch);
    EXPECT_EQ(shorter.header.request_id, kExampleRequestId);
    EXPECT_EQ(shorter.header.message_type, MessageType::SetWifiCredentials);
    expect_monostate(shorter);
}

TEST(MessageDecoder, UnsupportedVersionAndFlags)
{
    const std::array<uint8_t, kHeaderBytes> version_two{
        0x02, 0x01, 0x00, 0x34, 0x12, 0x00, 0x00,
    };
    Message version_message{};
    EXPECT_EQ(decode_message(version_two, version_message), Status::UnsupportedVersion);
    EXPECT_EQ(version_message.header.request_id, kExampleRequestId);
    EXPECT_EQ(version_message.header.message_type, MessageType::GetDeviceInfo);
    EXPECT_EQ(version_message.header.version, kUnsupportedVersion);
    expect_monostate(version_message);

    const std::array<uint8_t, kHeaderBytes> flags_set{
        0x01, 0x01, 0x01, 0x34, 0x12, 0x00, 0x00,
    };
    Message flags_message{};
    EXPECT_EQ(decode_message(flags_set, flags_message), Status::UnsupportedFlags);
    EXPECT_EQ(flags_message.header.request_id, kExampleRequestId);
    EXPECT_EQ(flags_message.header.flags, kUnsupportedFlag);
    expect_monostate(flags_message);
}

TEST(MessageDecoder, UnknownMessageTypeReportsRawType)
{
    const std::array<uint8_t, kHeaderBytes> bytes{
        kProtocolVersion, kUnassignedMessageType, kFlagsNone, 0x34, 0x12, 0x00, 0x00,
    };
    Message message{};
    EXPECT_EQ(decode_message(bytes, message), Status::UnknownMessageType);
    EXPECT_EQ(message.header.request_id, kExampleRequestId);
    EXPECT_EQ(static_cast<uint8_t>(message.header.message_type), kUnassignedMessageType);
    expect_monostate(message);
}

TEST(MessageDecoder, DecodeHeaderReadsPartialMessage)
{
    const std::array<uint8_t, kHeaderBytes> header_only{
        0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00,
    };
    MessageHeader header{};
    ASSERT_EQ(decode_header(header_only, header), Status::Ok);
    EXPECT_EQ(header.version, kProtocolVersion);
    EXPECT_EQ(header.message_type, MessageType::SetWifiCredentials);
    EXPECT_EQ(header.flags, kFlagsNone);
    EXPECT_EQ(header.request_id, kExampleRequestId);
    EXPECT_EQ(header.body_length, kExampleBodyLength);

    MessageHeader unchanged{};
    unchanged.request_id = kOtherUnchangedRequestId;
    EXPECT_EQ(decode_header(std::span<const uint8_t>(header_only.data(), kRequestIdOffset), unchanged),
              Status::MessageTooShort);
    EXPECT_EQ(unchanged.request_id, kOtherUnchangedRequestId);
}

TEST(MessageDecoder, ResetsBodyWhenWifiBodyIsRejected)
{
    const auto empty_ssid = std::to_array<uint8_t>({
        0x01, 0x02, 0x00, 0x34, 0x12, 0x01, 0x00, 0x00,
    });
    expect_body_rejected(empty_ssid, Status::InvalidField, MessageType::SetWifiCredentials, kExampleRequestId);

    const auto truncated = std::to_array<uint8_t>({
        0x01, 0x02, 0x00, 0x34, 0x12, 0x03, 0x00, 0x04, 0x68, 0x6F,
    });
    expect_body_rejected(truncated, Status::MalformedBody, MessageType::SetWifiCredentials, kExampleRequestId);
}
