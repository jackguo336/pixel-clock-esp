#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

#include "companion_protocol/message_decoder.hpp"
#include "companion_protocol/message_encoder.hpp"
#include "gtest/gtest.h"
#include "utils.hpp"

using companion_protocol_test::expect_same_bytes;
using companion_protocol_test::make_message;

namespace {

using companion_protocol::DeviceInfo;
using companion_protocol::Message;
using companion_protocol::MessageType;
using companion_protocol::Status;
using companion_protocol::decode_message;
using companion_protocol::encode_message;
using companion_protocol::kFirmwareVersionLengthPrefixBytes;
using companion_protocol::kHeaderBytes;
using companion_protocol::kMaxFirmwareVersionBytes;
using companion_protocol::kMaxMessageBytes;
using companion_protocol::kProtocolVersion;
using companion_protocol::kProtocolVersionFieldBytes;

constexpr uint16_t kExampleRequestId = 0x0001;

}  // namespace

TEST(DeviceInfo, DocumentedExampleMatchesEncoder)
{
    // Example message from components/companion_protocol/PROTOCOL.md.
    const auto device_info = std::to_array<uint8_t>({
        0x01, 0x81, 0x00, 0x01, 0x00, 0x07, 0x00, 0x01, 0x05, 0x31, 0x2E, 0x30, 0x2E, 0x30,
    });
    DeviceInfo info{};
    info.protocol_version = kProtocolVersion;
    ASSERT_TRUE(info.firmware_version.assign(std::string_view{"1.0.0"}));
    std::array<uint8_t, device_info.size()> encoded_info{};
    ASSERT_EQ(encode_message(make_message(kExampleRequestId, info), encoded_info), device_info.size());
    expect_same_bytes(encoded_info, device_info);

    Message decoded_info_message{};
    ASSERT_EQ(decode_message(device_info, decoded_info_message), Status::Ok);
    const DeviceInfo* decoded_info = std::get_if<DeviceInfo>(&decoded_info_message.body);
    ASSERT_NE(decoded_info, nullptr);
    EXPECT_EQ(decoded_info->protocol_version, kProtocolVersion);
    expect_same_bytes(decoded_info->firmware_version.view(), info.firmware_version.view());
}

TEST(DeviceInfo, RoundTripMaxFirmwareVersion)
{
    std::array<uint8_t, kMaxFirmwareVersionBytes> firmware_version{};
    for (std::size_t index = 0; index < firmware_version.size(); ++index) {
        firmware_version[index] = static_cast<uint8_t>(index + 1);
    }

    DeviceInfo info{};
    info.protocol_version = kProtocolVersion;
    ASSERT_TRUE(info.firmware_version.assign(firmware_version));

    std::array<uint8_t, kMaxMessageBytes> buffer{};
    const std::size_t length = encode_message(make_message(kExampleRequestId, info), buffer);
    ASSERT_EQ(length, kHeaderBytes + kProtocolVersionFieldBytes + kFirmwareVersionLengthPrefixBytes
                          + kMaxFirmwareVersionBytes);

    Message message{};
    ASSERT_EQ(decode_message(std::span<const uint8_t>(buffer.data(), length), message), Status::Ok);
    const DeviceInfo* decoded = std::get_if<DeviceInfo>(&message.body);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->protocol_version, kProtocolVersion);
    expect_same_bytes(decoded->firmware_version.view(), firmware_version);
}

TEST(DeviceInfo, RejectsMalformedBody)
{
    constexpr std::string_view kKeepVersion = "keep";
    DeviceInfo info{};
    info.protocol_version = 4;
    ASSERT_TRUE(info.firmware_version.assign(kKeepVersion));

    EXPECT_EQ(info.decode({}), Status::MalformedBody);
    EXPECT_EQ(info.decode(std::array<uint8_t, 1>{0x01}), Status::MalformedBody);
    const std::array<uint8_t, 2> version_too_long{0x01, static_cast<uint8_t>(kMaxFirmwareVersionBytes + 1)};
    EXPECT_EQ(info.decode(version_too_long), Status::InvalidField);
    const auto trailing_info = std::to_array<uint8_t>({0x01, 0x01, 'a', 0xFF});
    EXPECT_EQ(info.decode(trailing_info), Status::MalformedBody);
    EXPECT_EQ(info.protocol_version, 4);
    expect_same_bytes(info.firmware_version.view(),
                      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(kKeepVersion.data()),
                                               kKeepVersion.size()));
}
