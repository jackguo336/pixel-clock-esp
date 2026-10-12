#include <array>
#include <climits>
#include <cstdint>
#include <span>

#include "companion_protocol/frame.hpp"
#include "gtest/gtest.h"

namespace {

using companion_protocol::Frame;
using companion_protocol::FrameHeader;
using companion_protocol::MessageType;
using companion_protocol::Status;
using companion_protocol::decode_frame;
using companion_protocol::decode_header;
using companion_protocol::kBodyLengthOffset;
using companion_protocol::kFlagsNone;
using companion_protocol::kFlagsOffset;
using companion_protocol::kHeaderBytes;
using companion_protocol::kMaxFrameBytes;
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
constexpr uint16_t kLittleEndianBodyLength = 2;
constexpr uint16_t kExampleBodyLength = 0x000E;

}  // namespace

TEST(Frame, GetDeviceInfoHandBuiltFrameDecodes)
{
    const std::array<uint8_t, kHeaderBytes> bytes{
        0x01, 0x01, 0x00, 0x2A, 0x00, 0x00, 0x00,
    };

    Frame frame{};
    ASSERT_EQ(decode_frame(bytes, frame), Status::Ok);
    EXPECT_EQ(frame.header.version, kProtocolVersion);
    EXPECT_EQ(frame.header.message_type, MessageType::GetDeviceInfo);
    EXPECT_EQ(frame.header.flags, kFlagsNone);
    EXPECT_EQ(frame.header.request_id, 0x002Au);
    EXPECT_EQ(frame.header.body_length, 0u);
    EXPECT_TRUE(frame.body.empty());
    EXPECT_EQ(frame.body.data(), bytes.data() + kHeaderBytes);
}

TEST(Frame, MultiByteHeaderFieldsAreLittleEndian)
{
    // request_id 0x1234 is the bytes 34 12. body_length 0x0002 is the bytes 02 00.
    const std::array<uint8_t, kHeaderBytes + kLittleEndianBodyLength> bytes{
        0x01, 0x01, 0x00, 0x34, 0x12, 0x02, 0x00, 0xAA, 0xBB,
    };

    Frame frame{};
    ASSERT_EQ(decode_frame(bytes, frame), Status::Ok);
    EXPECT_EQ(frame.header.request_id, kExampleRequestId);
    EXPECT_EQ(frame.header.body_length, kLittleEndianBodyLength);
    ASSERT_EQ(frame.body.size(), kLittleEndianBodyLength);
    EXPECT_EQ(frame.body[0], 0xAAu);
    EXPECT_EQ(frame.body[1], 0xBBu);
}

TEST(Frame, ShortInputReturnsFrameTooShort)
{
    const std::array<uint8_t, kHeaderBytes - 1> bytes{
        0x01, 0x01, 0x00, 0x34, 0x12, 0x00,
    };
    Frame frame{};
    frame.header.request_id = kSentinelRequestId;
    frame.header.message_type = MessageType::Ack;
    frame.header.version = kSentinelVersion;

    EXPECT_EQ(decode_frame(bytes, frame), Status::FrameTooShort);
    EXPECT_EQ(frame.header.request_id, kSentinelRequestId);
    EXPECT_EQ(frame.header.message_type, MessageType::Ack);
    EXPECT_EQ(frame.header.version, kSentinelVersion);

    Frame empty_frame{};
    empty_frame.header.request_id = kUnchangedRequestId;
    EXPECT_EQ(decode_frame(std::span<const uint8_t>{}, empty_frame), Status::FrameTooShort);
    EXPECT_EQ(empty_frame.header.request_id, kUnchangedRequestId);
}

TEST(Frame, LongInputReturnsFrameTooLong)
{
    std::array<uint8_t, kMaxFrameBytes + 1> bytes{};
    bytes[kVersionOffset] = kProtocolVersion;
    bytes[kMessageTypeOffset] = static_cast<uint8_t>(MessageType::SetWifiCredentials);
    bytes[kFlagsOffset] = kFlagsNone;
    bytes[kRequestIdOffset] = static_cast<uint8_t>(kExampleRequestId);
    bytes[kRequestIdOffset + 1] = static_cast<uint8_t>(kExampleRequestId >> CHAR_BIT);

    Frame frame{};
    EXPECT_EQ(decode_frame(bytes, frame), Status::FrameTooLong);
    EXPECT_EQ(frame.header.request_id, kExampleRequestId);
    EXPECT_EQ(frame.header.message_type, MessageType::SetWifiCredentials);
    EXPECT_TRUE(frame.body.empty());

    std::array<uint8_t, kMaxFrameBytes> at_limit{};
    at_limit[kVersionOffset] = kProtocolVersion;
    at_limit[kMessageTypeOffset] = static_cast<uint8_t>(MessageType::GetDeviceInfo);
    at_limit[kFlagsOffset] = kFlagsNone;
    at_limit[kRequestIdOffset] = static_cast<uint8_t>(kExampleRequestId);
    at_limit[kRequestIdOffset + 1] = static_cast<uint8_t>(kExampleRequestId >> CHAR_BIT);
    const uint16_t body_length = static_cast<uint16_t>(kMaxFrameBytes - kHeaderBytes);
    at_limit[kBodyLengthOffset] = static_cast<uint8_t>(body_length);
    at_limit[kBodyLengthOffset + 1] = static_cast<uint8_t>(body_length >> CHAR_BIT);

    Frame limit_frame{};
    ASSERT_EQ(decode_frame(at_limit, limit_frame), Status::Ok);
    EXPECT_EQ(limit_frame.body.size(), body_length);
}

TEST(Frame, BodyLengthMismatchReportsRequestId)
{
    const std::array<uint8_t, kHeaderBytes> longer_than_bytes{
        0x01, 0x01, 0x00, 0x34, 0x12, 0x05, 0x00,
    };
    Frame longer{};
    EXPECT_EQ(decode_frame(longer_than_bytes, longer), Status::BodyLengthMismatch);
    EXPECT_EQ(longer.header.request_id, kExampleRequestId);
    EXPECT_EQ(longer.header.message_type, MessageType::GetDeviceInfo);

    const std::array<uint8_t, kHeaderBytes + 1> shorter_than_bytes{
        0x01, 0x02, 0x00, 0x34, 0x12, 0x00, 0x00, 0xFF,
    };
    Frame shorter{};
    EXPECT_EQ(decode_frame(shorter_than_bytes, shorter), Status::BodyLengthMismatch);
    EXPECT_EQ(shorter.header.request_id, kExampleRequestId);
    EXPECT_EQ(shorter.header.message_type, MessageType::SetWifiCredentials);
}

TEST(Frame, UnsupportedVersionAndFlags)
{
    const std::array<uint8_t, kHeaderBytes> version_two{
        0x02, 0x01, 0x00, 0x34, 0x12, 0x00, 0x00,
    };
    Frame version_frame{};
    EXPECT_EQ(decode_frame(version_two, version_frame), Status::UnsupportedVersion);
    EXPECT_EQ(version_frame.header.request_id, kExampleRequestId);
    EXPECT_EQ(version_frame.header.message_type, MessageType::GetDeviceInfo);
    EXPECT_EQ(version_frame.header.version, kUnsupportedVersion);

    const std::array<uint8_t, kHeaderBytes> flags_set{
        0x01, 0x01, 0x01, 0x34, 0x12, 0x00, 0x00,
    };
    Frame flags_frame{};
    EXPECT_EQ(decode_frame(flags_set, flags_frame), Status::UnsupportedFlags);
    EXPECT_EQ(flags_frame.header.request_id, kExampleRequestId);
    EXPECT_EQ(flags_frame.header.flags, kUnsupportedFlag);
}

TEST(Frame, UnknownMessageTypeReportsRawType)
{
    const std::array<uint8_t, kHeaderBytes> bytes{
        kProtocolVersion, kUnassignedMessageType, kFlagsNone, 0x34, 0x12, 0x00, 0x00,
    };
    Frame frame{};
    EXPECT_EQ(decode_frame(bytes, frame), Status::UnknownMessageType);
    EXPECT_EQ(frame.header.request_id, kExampleRequestId);
    EXPECT_EQ(static_cast<uint8_t>(frame.header.message_type), kUnassignedMessageType);
}

TEST(Frame, DecodeHeaderReadsPartialFrame)
{
    const std::array<uint8_t, kHeaderBytes> header_only{
        0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00,
    };
    FrameHeader header{};
    ASSERT_EQ(decode_header(header_only, header), Status::Ok);
    EXPECT_EQ(header.version, kProtocolVersion);
    EXPECT_EQ(header.message_type, MessageType::SetWifiCredentials);
    EXPECT_EQ(header.flags, kFlagsNone);
    EXPECT_EQ(header.request_id, kExampleRequestId);
    EXPECT_EQ(header.body_length, kExampleBodyLength);

    FrameHeader unchanged{};
    unchanged.request_id = kOtherUnchangedRequestId;
    EXPECT_EQ(decode_header(std::span<const uint8_t>(header_only.data(), kRequestIdOffset), unchanged),
              Status::FrameTooShort);
    EXPECT_EQ(unchanged.request_id, kOtherUnchangedRequestId);
}
