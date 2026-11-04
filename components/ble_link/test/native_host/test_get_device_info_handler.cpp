#include <array>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "ble_link/get_device_info_handler.hpp"
#include "gtest/gtest.h"

namespace {

class RecordingMessageTransport : public ble_link::MessageTransport {
public:
    bool send(std::span<const uint8_t> message) override
    {
        ++send_count;
        sent.assign(message.begin(), message.end());
        return true;
    }

    int send_count{0};
    std::vector<uint8_t> sent;
};

template <typename Body>
companion_protocol::Message request_with(Body body, uint16_t request_id)
{
    companion_protocol::Message request{};
    request.header.request_id = request_id;
    request.body = std::move(body);
    return request;
}

const uint8_t kFirmwareVersion[] = {'1', '.', '0', '.', '0'};

const std::vector<uint8_t> kDeviceInfoExample{
    0x01, 0x81, 0x00, 0x01, 0x00, 0x07, 0x00, 0x01, 0x05, 0x31, 0x2E, 0x30, 0x2E, 0x30,
};

}  // namespace

TEST(GetDeviceInfoHandler, RepliesWithProtocolExample)
{
    RecordingMessageTransport transport;
    ble_link::GetDeviceInfoHandler handler(transport, kFirmwareVersion);

    const companion_protocol::Message request =
        request_with(companion_protocol::GetDeviceInfo{}, 0x0001);
    EXPECT_TRUE(handler.try_handle(request));
    EXPECT_EQ(transport.send_count, 1);
    EXPECT_EQ(transport.sent, kDeviceInfoExample);
}

TEST(GetDeviceInfoHandler, EchoesRequestIdZero)
{
    RecordingMessageTransport transport;
    ble_link::GetDeviceInfoHandler handler(transport, kFirmwareVersion);

    EXPECT_TRUE(handler.try_handle(request_with(companion_protocol::GetDeviceInfo{}, 0)));
    const std::vector<uint8_t> expected{
        0x01, 0x81, 0x00, 0x00, 0x00, 0x07, 0x00, 0x01, 0x05, 0x31, 0x2E, 0x30, 0x2E, 0x30,
    };
    EXPECT_EQ(transport.sent, expected);
}

TEST(GetDeviceInfoHandler, OverlongFirmwareVersionStaysEmpty)
{
    RecordingMessageTransport transport;
    std::array<uint8_t, companion_protocol::kMaxFirmwareVersionBytes + 1> firmware_version{};
    firmware_version.fill('1');
    ble_link::GetDeviceInfoHandler handler(transport, firmware_version);

    EXPECT_TRUE(handler.try_handle(request_with(companion_protocol::GetDeviceInfo{}, 0x0001)));
    const std::vector<uint8_t> expected{0x01, 0x81, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00};
    EXPECT_EQ(transport.sent, expected);
}

TEST(GetDeviceInfoHandler, IgnoresOtherBodies)
{
    RecordingMessageTransport transport;
    ble_link::GetDeviceInfoHandler handler(transport, kFirmwareVersion);

    EXPECT_FALSE(handler.try_handle(request_with(companion_protocol::SetWifiCredentials{}, 0x0001)));
    EXPECT_FALSE(handler.try_handle(request_with(companion_protocol::Ack{}, 0x0001)));
    EXPECT_FALSE(handler.try_handle(request_with(companion_protocol::DeviceInfo{}, 0x0001)));
    EXPECT_FALSE(handler.try_handle(request_with(companion_protocol::Error{}, 0x0001)));
    EXPECT_EQ(transport.send_count, 0);
}
