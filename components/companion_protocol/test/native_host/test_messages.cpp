#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "companion_protocol/messages.hpp"
#include "gtest/gtest.h"

namespace {

using companion_protocol::Ack;
using companion_protocol::BoundedBytes;
using companion_protocol::DeviceInfo;
using companion_protocol::Error;
using companion_protocol::Frame;
using companion_protocol::MessageType;
using companion_protocol::SetWifiCredentials;
using companion_protocol::Status;
using companion_protocol::decode_ack;
using companion_protocol::decode_device_info;
using companion_protocol::decode_error;
using companion_protocol::decode_frame;
using companion_protocol::decode_get_device_info;
using companion_protocol::decode_set_wifi_credentials;
using companion_protocol::encode_ack;
using companion_protocol::encode_device_info;
using companion_protocol::encode_error;
using companion_protocol::encode_get_device_info;
using companion_protocol::encode_set_wifi_credentials;
using companion_protocol::kHeaderBytes;
using companion_protocol::kMaxFrameBytes;
using companion_protocol::kProtocolVersion;

void expect_same_bytes(std::span<const uint8_t> actual, std::span<const uint8_t> expected)
{
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(actual[index], expected[index]) << "at index " << index;
    }
}

[[nodiscard]] bool round_trip_wifi(const SetWifiCredentials& original, uint16_t request_id,
                                    SetWifiCredentials& decoded)
{
    std::array<uint8_t, kMaxFrameBytes> buffer{};
    const std::size_t length = encode_set_wifi_credentials(request_id, original, buffer);
    if (length == 0) {
        ADD_FAILURE() << "encode_set_wifi_credentials failed";
        return false;
    }

    Frame frame{};
    if (decode_frame(std::span<const uint8_t>(buffer.data(), length), frame) != Status::Ok) {
        ADD_FAILURE() << "decode_frame failed";
        return false;
    }
    EXPECT_EQ(frame.header.version, kProtocolVersion);
    EXPECT_EQ(frame.header.flags, 0);
    EXPECT_EQ(frame.header.request_id, request_id);
    EXPECT_EQ(frame.header.message_type, MessageType::SetWifiCredentials);
    EXPECT_EQ(frame.body.data(), buffer.data() + kHeaderBytes);

    if (decode_set_wifi_credentials(frame.body, decoded) != Status::Ok) {
        ADD_FAILURE() << "decode_set_wifi_credentials failed";
        return false;
    }
    return true;
}

template <typename Encode>
void expect_one_byte_short_writes_nothing(Encode&& encode)
{
    std::array<uint8_t, kMaxFrameBytes> full{};
    const std::size_t length = encode(std::span<uint8_t>(full));
    if (length == 0 || length > full.size()) {
        ADD_FAILURE() << "encoder did not produce a frame";
        return;
    }

    std::array<uint8_t, kMaxFrameBytes + 1> storage{};
    storage.fill(0x5A);
    const std::size_t short_length = length - 1;
    const std::size_t written = encode(std::span<uint8_t>(storage.data(), short_length));
    EXPECT_EQ(written, 0u);
    EXPECT_EQ(storage[short_length], 0x5A);
}

}  // namespace

TEST(Messages, SetWifiCredentialsRoundTripMaxLengths)
{
    std::array<uint8_t, 32> ssid{};
    std::array<uint8_t, 64> passphrase{};
    for (std::size_t index = 0; index < ssid.size(); ++index) {
        ssid[index] = static_cast<uint8_t>(index + 1);
    }
    for (std::size_t index = 0; index < passphrase.size(); ++index) {
        passphrase[index] = static_cast<uint8_t>(0x40 + (index % 50));
    }

    SetWifiCredentials original{};
    ASSERT_TRUE(original.ssid.assign(ssid));
    ASSERT_TRUE(original.passphrase.assign(passphrase));

    std::array<uint8_t, kMaxFrameBytes> buffer{};
    const std::size_t length = encode_set_wifi_credentials(0x1234, original, buffer);
    EXPECT_EQ(length, 105u);

    SetWifiCredentials decoded{};
    ASSERT_TRUE(round_trip_wifi(original, 0x1234, decoded));
    expect_same_bytes(decoded.ssid.view(), ssid);
    expect_same_bytes(decoded.passphrase.view(), passphrase);
}

TEST(Messages, SetWifiCredentialsRoundTripBinarySsid)
{
    const std::array<uint8_t, 4> ssid{0x00, 0xFF, 0xC3, 0xA9};
    const std::array<uint8_t, 3> passphrase{'k', 'e', 'y'};

    SetWifiCredentials original{};
    ASSERT_TRUE(original.ssid.assign(ssid));
    ASSERT_TRUE(original.passphrase.assign(passphrase));

    SetWifiCredentials decoded{};
    ASSERT_TRUE(round_trip_wifi(original, 0x0102, decoded));
    expect_same_bytes(decoded.ssid.view(), ssid);
    expect_same_bytes(decoded.passphrase.view(), passphrase);
}

TEST(Messages, SetWifiCredentialsRoundTripEmptyPassphrase)
{
    const std::array<uint8_t, 4> ssid{'h', 'o', 'm', 'e'};
    SetWifiCredentials original{};
    ASSERT_TRUE(original.ssid.assign(ssid));
    ASSERT_TRUE(original.passphrase.assign(std::string_view{}));

    SetWifiCredentials decoded{};
    ASSERT_TRUE(round_trip_wifi(original, 0x0007, decoded));
    expect_same_bytes(decoded.ssid.view(), ssid);
    EXPECT_EQ(decoded.passphrase.size(), 0u);
}

TEST(Messages, DocumentedExamplesMatchEncoder)
{
    // Golden frames from components/companion_protocol/PROTOCOL.md.
    const std::array<uint8_t, 7> get_device_info{
        0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    };
    std::array<uint8_t, 7> encoded_get{};
    ASSERT_EQ(encode_get_device_info(0x0001, encoded_get), get_device_info.size());
    expect_same_bytes(encoded_get, get_device_info);

    const std::array<uint8_t, 21> set_wifi{
        0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00, 0x04, 0x68, 0x6F, 0x6D,
        0x65, 0x08, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6F, 0x72, 0x64,
    };
    SetWifiCredentials credentials{};
    ASSERT_TRUE(credentials.ssid.assign(std::string_view{"home"}));
    ASSERT_TRUE(credentials.passphrase.assign(std::string_view{"password"}));
    std::array<uint8_t, 21> encoded_wifi{};
    ASSERT_EQ(encode_set_wifi_credentials(0x1234, credentials, encoded_wifi), set_wifi.size());
    expect_same_bytes(encoded_wifi, set_wifi);

    Frame wifi_frame{};
    ASSERT_EQ(decode_frame(set_wifi, wifi_frame), Status::Ok);
    SetWifiCredentials decoded_wifi{};
    ASSERT_EQ(decode_set_wifi_credentials(wifi_frame.body, decoded_wifi), Status::Ok);
    expect_same_bytes(decoded_wifi.ssid.view(), credentials.ssid.view());
    expect_same_bytes(decoded_wifi.passphrase.view(), credentials.passphrase.view());

    const std::array<uint8_t, 8> ack_ok{
        0x01, 0x80, 0x00, 0x34, 0x12, 0x01, 0x00, 0x00,
    };
    std::array<uint8_t, 8> encoded_ack{};
    ASSERT_EQ(encode_ack(0x1234, Ack{}, encoded_ack), ack_ok.size());
    expect_same_bytes(encoded_ack, ack_ok);

    const std::array<uint8_t, 14> device_info{
        0x01, 0x81, 0x00, 0x01, 0x00, 0x07, 0x00, 0x01, 0x05, 0x31, 0x2E, 0x30, 0x2E, 0x30,
    };
    DeviceInfo info{};
    info.protocol_version = 1;
    ASSERT_TRUE(info.firmware_version.assign(std::string_view{"1.0.0"}));
    std::array<uint8_t, 14> encoded_info{};
    ASSERT_EQ(encode_device_info(0x0001, info, encoded_info), device_info.size());
    expect_same_bytes(encoded_info, device_info);

    const std::array<uint8_t, 9> error_frame{
        0x01, 0xFF, 0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x02,
    };
    const Error error{
        .code = Status::InvalidField,
        .failed_message_type = MessageType::SetWifiCredentials,
    };
    std::array<uint8_t, 9> encoded_error{};
    ASSERT_EQ(encode_error(0, error, encoded_error), error_frame.size());
    expect_same_bytes(encoded_error, error_frame);
}

TEST(Messages, BoundedBytesAssignRejectsOverflow)
{
    BoundedBytes<32> field{};
    ASSERT_TRUE(field.assign(std::string_view{"previous"}));

    std::array<uint8_t, 33> too_long{};
    too_long.fill(0xAB);
    EXPECT_FALSE(field.assign(std::span<const uint8_t>(too_long)));
    EXPECT_EQ(field.size(), 8u);
    expect_same_bytes(field.view(), std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("previous"), 8));

    EXPECT_FALSE(field.assign(std::string_view{"012345678901234567890123456789012"}));
    EXPECT_EQ(field.size(), 8u);
    EXPECT_EQ(field.capacity(), 32u);
}

TEST(Messages, SetWifiCredentialsInvalidFieldLengths)
{
    SetWifiCredentials credentials{};
    ASSERT_TRUE(credentials.ssid.assign(std::string_view{"keep"}));
    ASSERT_TRUE(credentials.passphrase.assign(std::string_view{"secret"}));

    const std::array<uint8_t, 1> empty_ssid{0x00};
    EXPECT_EQ(decode_set_wifi_credentials(empty_ssid, credentials), Status::InvalidField);
    expect_same_bytes(credentials.ssid.view(), std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("keep"), 4));

    const std::array<uint8_t, 1> ssid_too_long{33};
    EXPECT_EQ(decode_set_wifi_credentials(ssid_too_long, credentials), Status::InvalidField);

    const std::array<uint8_t, 3> passphrase_too_long{0x01, 'a', 65};
    EXPECT_EQ(decode_set_wifi_credentials(passphrase_too_long, credentials), Status::InvalidField);
    expect_same_bytes(credentials.passphrase.view(),
                      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("secret"), 6));
}

TEST(Messages, SetWifiCredentialsMalformedBody)
{
    SetWifiCredentials credentials{};
    const std::array<uint8_t, 3> truncated{0x04, 'h', 'o'};
    EXPECT_EQ(decode_set_wifi_credentials(truncated, credentials), Status::MalformedBody);

    const std::array<uint8_t, 7> trailing{0x04, 'h', 'o', 'm', 'e', 0x00, 0xFF};
    EXPECT_EQ(decode_set_wifi_credentials(trailing, credentials), Status::MalformedBody);

    EXPECT_EQ(decode_set_wifi_credentials({}, credentials), Status::MalformedBody);
}

TEST(Messages, GetDeviceInfoRejectsNonEmptyBody)
{
    EXPECT_EQ(decode_get_device_info({}), Status::Ok);

    const std::array<uint8_t, 8> frame_bytes{
        0x01, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xFF,
    };
    Frame frame{};
    ASSERT_EQ(decode_frame(frame_bytes, frame), Status::Ok);
    EXPECT_EQ(decode_get_device_info(frame.body), Status::MalformedBody);
}

TEST(Messages, DeviceInfoRoundTrip)
{
    constexpr uint8_t kReportedProtocolVersion = 4;
    DeviceInfo info{};
    info.protocol_version = kReportedProtocolVersion;
    ASSERT_TRUE(info.firmware_version.assign(std::string_view{"pixel-clock"}));

    std::array<uint8_t, kMaxFrameBytes> buffer{};
    const std::size_t length = encode_device_info(0x0042, info, buffer);
    ASSERT_GT(length, 0u);

    Frame frame{};
    ASSERT_EQ(decode_frame(std::span<const uint8_t>(buffer.data(), length), frame), Status::Ok);
    EXPECT_EQ(frame.header.request_id, 0x0042u);
    EXPECT_EQ(frame.header.version, kProtocolVersion);
    EXPECT_EQ(frame.header.message_type, MessageType::DeviceInfo);

    DeviceInfo decoded{};
    ASSERT_EQ(decode_device_info(frame.body, decoded), Status::Ok);
    EXPECT_EQ(decoded.protocol_version, kReportedProtocolVersion);
    expect_same_bytes(decoded.firmware_version.view(), info.firmware_version.view());
}

TEST(Messages, AckAndErrorCarryRequestId)
{
    const Ack ack{.status = Status::StorageFailed};
    std::array<uint8_t, kMaxFrameBytes> buffer{};
    const std::size_t ack_length = encode_ack(0x1234, ack, buffer);
    ASSERT_GT(ack_length, 0u);

    Frame ack_frame{};
    ASSERT_EQ(decode_frame(std::span<const uint8_t>(buffer.data(), ack_length), ack_frame), Status::Ok);
    EXPECT_EQ(ack_frame.header.request_id, 0x1234u);
    EXPECT_EQ(ack_frame.header.message_type, MessageType::Ack);
    Ack decoded_ack{};
    ASSERT_EQ(decode_ack(ack_frame.body, decoded_ack), Status::Ok);
    EXPECT_EQ(decoded_ack.status, Status::StorageFailed);

    const Error error{
        .code = Status::InvalidField,
        .failed_message_type = MessageType::SetWifiCredentials,
    };
    const std::size_t error_length = encode_error(0x0ABC, error, buffer);
    ASSERT_GT(error_length, 0u);

    Frame error_frame{};
    ASSERT_EQ(decode_frame(std::span<const uint8_t>(buffer.data(), error_length), error_frame), Status::Ok);
    EXPECT_EQ(error_frame.header.request_id, 0x0ABCu);
    EXPECT_EQ(error_frame.header.message_type, MessageType::Error);
    Error decoded_error{};
    ASSERT_EQ(decode_error(error_frame.body, decoded_error), Status::Ok);
    EXPECT_EQ(decoded_error.code, Status::InvalidField);
    EXPECT_EQ(decoded_error.failed_message_type, MessageType::SetWifiCredentials);
}

TEST(Messages, EncodersRejectShortBuffer)
{
    expect_one_byte_short_writes_nothing([](std::span<uint8_t> out) { return encode_get_device_info(0x0001, out); });

    SetWifiCredentials credentials{};
    ASSERT_TRUE(credentials.ssid.assign(std::string_view{"home"}));
    ASSERT_TRUE(credentials.passphrase.assign(std::string_view{"password"}));
    expect_one_byte_short_writes_nothing(
        [&](std::span<uint8_t> out) { return encode_set_wifi_credentials(0x1234, credentials, out); });

    DeviceInfo info{};
    ASSERT_TRUE(info.firmware_version.assign(std::string_view{"1.0.0"}));
    expect_one_byte_short_writes_nothing([&](std::span<uint8_t> out) { return encode_device_info(0x0001, info, out); });

    expect_one_byte_short_writes_nothing([](std::span<uint8_t> out) { return encode_ack(0x1234, Ack{}, out); });

    const Error error{
        .code = Status::InvalidField,
        .failed_message_type = MessageType::GetDeviceInfo,
    };
    expect_one_byte_short_writes_nothing([&](std::span<uint8_t> out) { return encode_error(0, error, out); });
}
