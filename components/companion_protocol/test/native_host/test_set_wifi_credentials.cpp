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
using companion_protocol_test::expect_unwritten;
using companion_protocol_test::kUnwrittenByte;
using companion_protocol_test::make_message;

namespace {

using companion_protocol::Message;
using companion_protocol::MessageType;
using companion_protocol::SetWifiCredentials;
using companion_protocol::Status;
using companion_protocol::decode_message;
using companion_protocol::encode_message;
using companion_protocol::kFlagsNone;
using companion_protocol::kHeaderBytes;
using companion_protocol::kMaxMessageBytes;
using companion_protocol::kMaxPassphraseBytes;
using companion_protocol::kMaxSsidBytes;
using companion_protocol::kMinSsidBytes;
using companion_protocol::kPassphraseLengthPrefixBytes;
using companion_protocol::kProtocolVersion;
using companion_protocol::kSsidLengthPrefixBytes;

constexpr uint16_t kExampleRequestId = 0x1234;

[[nodiscard]] bool round_trip_wifi(const SetWifiCredentials& original, uint16_t request_id,
                                    SetWifiCredentials& decoded)
{
    std::array<uint8_t, kMaxMessageBytes> buffer{};
    const std::size_t length = encode_message(make_message(request_id, original), buffer);
    if (length == 0) {
        ADD_FAILURE() << "encode_message failed";
        return false;
    }

    Message message{};
    if (decode_message(std::span<const uint8_t>(buffer.data(), length), message) != Status::Ok) {
        ADD_FAILURE() << "decode_message failed";
        return false;
    }
    EXPECT_EQ(message.header.version, kProtocolVersion);
    EXPECT_EQ(message.header.flags, kFlagsNone);
    EXPECT_EQ(message.header.request_id, request_id);
    EXPECT_EQ(message.header.message_type, MessageType::SetWifiCredentials);

    const SetWifiCredentials* body = std::get_if<SetWifiCredentials>(&message.body);
    if (body == nullptr) {
        ADD_FAILURE() << "decoded body is not SetWifiCredentials";
        return false;
    }
    decoded = *body;
    return true;
}

}  // namespace

TEST(SetWifiCredentials, RoundTripMaxLengths)
{
    std::array<uint8_t, kMaxSsidBytes> ssid{};
    std::array<uint8_t, kMaxPassphraseBytes> passphrase{};
    for (std::size_t index = 0; index < ssid.size(); ++index) {
        ssid[index] = static_cast<uint8_t>(index + 1);
    }
    for (std::size_t index = 0; index < passphrase.size(); ++index) {
        passphrase[index] = static_cast<uint8_t>(index);
    }

    SetWifiCredentials original{};
    ASSERT_TRUE(original.ssid.assign(ssid));
    ASSERT_TRUE(original.passphrase.assign(passphrase));

    std::array<uint8_t, kMaxMessageBytes> buffer{};
    const std::size_t length = encode_message(make_message(kExampleRequestId, original), buffer);
    ASSERT_EQ(length, kHeaderBytes + kSsidLengthPrefixBytes + kMaxSsidBytes + kPassphraseLengthPrefixBytes
                          + kMaxPassphraseBytes);

    SetWifiCredentials decoded{};
    ASSERT_TRUE(round_trip_wifi(original, kExampleRequestId, decoded));
    expect_same_bytes(decoded.ssid.view(), ssid);
    expect_same_bytes(decoded.passphrase.view(), passphrase);
}

TEST(SetWifiCredentials, RoundTripEmptyPassphrase)
{
    const auto ssid = std::to_array<uint8_t>({'h', 'o', 'm', 'e'});
    SetWifiCredentials original{};
    ASSERT_TRUE(original.ssid.assign(ssid));
    ASSERT_TRUE(original.passphrase.assign(std::string_view{}));

    SetWifiCredentials decoded{};
    ASSERT_TRUE(round_trip_wifi(original, 0x0007, decoded));
    expect_same_bytes(decoded.ssid.view(), ssid);
    EXPECT_EQ(decoded.passphrase.size(), 0u);
}

TEST(SetWifiCredentials, DocumentedExampleMatchesEncoder)
{
    // Example message from components/companion_protocol/PROTOCOL.md.
    const auto set_wifi = std::to_array<uint8_t>({
        0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00, 0x04, 0x68, 0x6F, 0x6D,
        0x65, 0x08, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6F, 0x72, 0x64,
    });
    SetWifiCredentials credentials{};
    ASSERT_TRUE(credentials.ssid.assign(std::string_view{"home"}));
    ASSERT_TRUE(credentials.passphrase.assign(std::string_view{"password"}));
    std::array<uint8_t, set_wifi.size()> encoded_wifi{};
    ASSERT_EQ(encode_message(make_message(kExampleRequestId, credentials), encoded_wifi), set_wifi.size());
    expect_same_bytes(encoded_wifi, set_wifi);

    Message wifi_message{};
    ASSERT_EQ(decode_message(set_wifi, wifi_message), Status::Ok);
    const SetWifiCredentials* decoded_wifi = std::get_if<SetWifiCredentials>(&wifi_message.body);
    ASSERT_NE(decoded_wifi, nullptr);
    expect_same_bytes(decoded_wifi->ssid.view(), credentials.ssid.view());
    expect_same_bytes(decoded_wifi->passphrase.view(), credentials.passphrase.view());
}

TEST(SetWifiCredentials, InvalidFieldLengths)
{
    constexpr std::string_view kKeep = "keep";
    constexpr std::string_view kSecret = "secret";
    SetWifiCredentials credentials{};
    ASSERT_TRUE(credentials.ssid.assign(kKeep));
    ASSERT_TRUE(credentials.passphrase.assign(kSecret));

    const std::array<uint8_t, kSsidLengthPrefixBytes> empty_ssid{0x00};
    EXPECT_EQ(credentials.decode(empty_ssid), Status::InvalidField);
    expect_same_bytes(credentials.ssid.view(),
                      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(kKeep.data()), kKeep.size()));

    const std::array<uint8_t, kSsidLengthPrefixBytes> ssid_too_long{static_cast<uint8_t>(kMaxSsidBytes + 1)};
    EXPECT_EQ(credentials.decode(ssid_too_long), Status::InvalidField);

    const std::array<uint8_t, kSsidLengthPrefixBytes + kMinSsidBytes + kPassphraseLengthPrefixBytes> passphrase_too_long{
        static_cast<uint8_t>(kMinSsidBytes), 'a', static_cast<uint8_t>(kMaxPassphraseBytes + 1)};
    EXPECT_EQ(credentials.decode(passphrase_too_long), Status::InvalidField);
    expect_same_bytes(credentials.passphrase.view(),
                      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(kSecret.data()), kSecret.size()));
}

TEST(SetWifiCredentials, MalformedBody)
{
    SetWifiCredentials credentials{};
    const auto truncated = std::to_array<uint8_t>({0x04, 'h', 'o'});
    EXPECT_EQ(credentials.decode(truncated), Status::MalformedBody);

    const auto trailing = std::to_array<uint8_t>({0x04, 'h', 'o', 'm', 'e', 0x00, 0xFF});
    EXPECT_EQ(credentials.decode(trailing), Status::MalformedBody);

    EXPECT_EQ(credentials.decode({}), Status::MalformedBody);
}

TEST(SetWifiCredentials, EncodeRejectsEmptySsid)
{
    std::array<uint8_t, kMaxMessageBytes> buffer{};
    buffer.fill(kUnwrittenByte);
    EXPECT_EQ(encode_message(make_message(kExampleRequestId, SetWifiCredentials{}), buffer), 0u);
    expect_unwritten(buffer);
}
