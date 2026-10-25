#include <array>
#include <cstdint>
#include <variant>

#include "companion_protocol/message_decoder.hpp"
#include "companion_protocol/message_encoder.hpp"
#include "gtest/gtest.h"
#include "utils.hpp"

using companion_protocol_test::expect_body_rejected;
using companion_protocol_test::expect_same_bytes;
using companion_protocol_test::make_message;

namespace {

using companion_protocol::GetDeviceInfo;
using companion_protocol::Message;
using companion_protocol::MessageType;
using companion_protocol::Status;
using companion_protocol::decode_message;
using companion_protocol::encode_message;

}  // namespace

TEST(GetDeviceInfo, DocumentedExampleMatchesEncoder)
{
    // Example message from components/companion_protocol/PROTOCOL.md.
    const auto get_device_info = std::to_array<uint8_t>({
        0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    });
    std::array<uint8_t, get_device_info.size()> encoded_get{};
    ASSERT_EQ(encode_message(make_message(0x0001, GetDeviceInfo{}), encoded_get), get_device_info.size());
    expect_same_bytes(encoded_get, get_device_info);

    Message decoded_get{};
    ASSERT_EQ(decode_message(get_device_info, decoded_get), Status::Ok);
    EXPECT_EQ(decoded_get.header.request_id, 0x0001u);
    EXPECT_TRUE(std::holds_alternative<GetDeviceInfo>(decoded_get.body));
}

TEST(GetDeviceInfo, RejectsNonEmptyBody)
{
    EXPECT_EQ(GetDeviceInfo{}.decode({}), Status::Ok);
    EXPECT_EQ(GetDeviceInfo{}.decode(std::array<uint8_t, 1>{0xFF}), Status::MalformedBody);

    const auto message_bytes = std::to_array<uint8_t>({
        0x01, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xFF,
    });
    expect_body_rejected(message_bytes, Status::MalformedBody, MessageType::GetDeviceInfo, 0x0001);
}
