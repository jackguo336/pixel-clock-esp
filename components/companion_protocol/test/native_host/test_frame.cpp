#include <array>
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
using companion_protocol::kHeaderBytes;
using companion_protocol::kMaxFrameBytes;
using companion_protocol::kProtocolVersion;

}  // namespace

TEST(Frame, GetDeviceInfoHandBuiltFrameDecodes)
{
    const std::array<uint8_t, 7> bytes{
        0x01, 0x01, 0x00, 0x2A, 0x00, 0x00, 0x00,
    };

    Frame frame{};
    ASSERT_EQ(decode_frame(bytes, frame), Status::Ok);
    EXPECT_EQ(frame.header.version, kProtocolVersion);
    EXPECT_EQ(frame.header.message_type, MessageType::GetDeviceInfo);
    EXPECT_EQ(frame.header.flags, 0);
    EXPECT_EQ(frame.header.request_id, 0x002Au);
    EXPECT_EQ(frame.header.body_length, 0u);
    EXPECT_TRUE(frame.body.empty());
    EXPECT_EQ(frame.body.data(), bytes.data() + kHeaderBytes);
}

TEST(Frame, MultiByteHeaderFieldsAreLittleEndian)
{
    // request_id 0x1234 is the bytes 34 12. body_length 0x0002 is the bytes 02 00.
    const std::array<uint8_t, 9> bytes{
        0x01, 0x01, 0x00, 0x34, 0x12, 0x02, 0x00, 0xAA, 0xBB,
    };

    Frame frame{};
    ASSERT_EQ(decode_frame(bytes, frame), Status::Ok);
    EXPECT_EQ(frame.header.request_id, 0x1234u);
    EXPECT_EQ(frame.header.body_length, 2u);
    ASSERT_EQ(frame.body.size(), 2u);
    EXPECT_EQ(frame.body[0], 0xAAu);
    EXPECT_EQ(frame.body[1], 0xBBu);
}

TEST(Frame, ShortInputReturnsFrameTooShort)
{
    const std::array<uint8_t, 6> bytes{
        0x01, 0x01, 0x00, 0x34, 0x12, 0x00,
    };
    Frame frame{};
    frame.header.request_id = 0xFFFF;
    frame.header.message_type = MessageType::Ack;
    frame.header.version = 9;

    EXPECT_EQ(decode_frame(bytes, frame), Status::FrameTooShort);
    EXPECT_EQ(frame.header.request_id, 0xFFFFu);
    EXPECT_EQ(frame.header.message_type, MessageType::Ack);
    EXPECT_EQ(frame.header.version, 9);

    Frame empty_frame{};
    empty_frame.header.request_id = 7;
    EXPECT_EQ(decode_frame(std::span<const uint8_t>{}, empty_frame), Status::FrameTooShort);
    EXPECT_EQ(empty_frame.header.request_id, 7u);
}

TEST(Frame, LongInputReturnsFrameTooLong)
{
    std::array<uint8_t, kMaxFrameBytes + 1> bytes{};
    bytes[0] = kProtocolVersion;
    bytes[1] = static_cast<uint8_t>(MessageType::SetWifiCredentials);
    bytes[2] = 0;
    bytes[3] = 0x34;
    bytes[4] = 0x12;

    Frame frame{};
    EXPECT_EQ(decode_frame(bytes, frame), Status::FrameTooLong);
    EXPECT_EQ(frame.header.request_id, 0x1234u);
    EXPECT_EQ(frame.header.message_type, MessageType::SetWifiCredentials);
    EXPECT_TRUE(frame.body.empty());

    std::array<uint8_t, kMaxFrameBytes> at_limit{};
    at_limit[0] = kProtocolVersion;
    at_limit[1] = static_cast<uint8_t>(MessageType::GetDeviceInfo);
    at_limit[2] = 0;
    at_limit[3] = 0x34;
    at_limit[4] = 0x12;
    const uint16_t body_length = static_cast<uint16_t>(kMaxFrameBytes - kHeaderBytes);
    at_limit[5] = static_cast<uint8_t>(body_length & 0xFFu);
    at_limit[6] = static_cast<uint8_t>((body_length >> 8) & 0xFFu);

    Frame limit_frame{};
    ASSERT_EQ(decode_frame(at_limit, limit_frame), Status::Ok);
    EXPECT_EQ(limit_frame.body.size(), body_length);
}

TEST(Frame, BodyLengthMismatchReportsRequestId)
{
    const std::array<uint8_t, 7> longer_than_bytes{
        0x01, 0x01, 0x00, 0x34, 0x12, 0x05, 0x00,
    };
    Frame longer{};
    EXPECT_EQ(decode_frame(longer_than_bytes, longer), Status::BodyLengthMismatch);
    EXPECT_EQ(longer.header.request_id, 0x1234u);
    EXPECT_EQ(longer.header.message_type, MessageType::GetDeviceInfo);

    const std::array<uint8_t, 8> shorter_than_bytes{
        0x01, 0x02, 0x00, 0x34, 0x12, 0x00, 0x00, 0xFF,
    };
    Frame shorter{};
    EXPECT_EQ(decode_frame(shorter_than_bytes, shorter), Status::BodyLengthMismatch);
    EXPECT_EQ(shorter.header.request_id, 0x1234u);
    EXPECT_EQ(shorter.header.message_type, MessageType::SetWifiCredentials);
}

TEST(Frame, UnsupportedVersionAndFlags)
{
    const std::array<uint8_t, 7> version_two{
        0x02, 0x01, 0x00, 0x34, 0x12, 0x00, 0x00,
    };
    Frame version_frame{};
    EXPECT_EQ(decode_frame(version_two, version_frame), Status::UnsupportedVersion);
    EXPECT_EQ(version_frame.header.request_id, 0x1234u);
    EXPECT_EQ(version_frame.header.message_type, MessageType::GetDeviceInfo);
    EXPECT_EQ(version_frame.header.version, 2);

    const std::array<uint8_t, 7> flags_set{
        0x01, 0x01, 0x01, 0x34, 0x12, 0x00, 0x00,
    };
    Frame flags_frame{};
    EXPECT_EQ(decode_frame(flags_set, flags_frame), Status::UnsupportedFlags);
    EXPECT_EQ(flags_frame.header.request_id, 0x1234u);
    EXPECT_EQ(flags_frame.header.flags, 1);
}

TEST(Frame, UnknownMessageTypeReportsRawType)
{
    const std::array<uint8_t, 7> bytes{
        0x01, 0x42, 0x00, 0x34, 0x12, 0x00, 0x00,
    };
    Frame frame{};
    EXPECT_EQ(decode_frame(bytes, frame), Status::UnknownMessageType);
    EXPECT_EQ(frame.header.request_id, 0x1234u);
    EXPECT_EQ(static_cast<uint8_t>(frame.header.message_type), 0x42);
}

TEST(Frame, DecodeHeaderReadsPartialFrame)
{
    const std::array<uint8_t, 7> header_only{
        0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00,
    };
    FrameHeader header{};
    ASSERT_EQ(decode_header(header_only, header), Status::Ok);
    EXPECT_EQ(header.version, kProtocolVersion);
    EXPECT_EQ(header.message_type, MessageType::SetWifiCredentials);
    EXPECT_EQ(header.flags, 0);
    EXPECT_EQ(header.request_id, 0x1234u);
    EXPECT_EQ(header.body_length, 14u);

    FrameHeader unchanged{};
    unchanged.request_id = 99;
    const std::array<uint8_t, 3> too_short{0x01, 0x02, 0x00};
    EXPECT_EQ(decode_header(too_short, unchanged), Status::FrameTooShort);
    EXPECT_EQ(unchanged.request_id, 99u);
}
