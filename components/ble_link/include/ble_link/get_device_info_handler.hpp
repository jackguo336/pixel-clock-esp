#pragma once

#include <span>

#include "ble_link/inbound_message_handler.hpp"
#include "companion_protocol/device_info.hpp"

namespace ble_link {

class GetDeviceInfoHandler : public InboundMessageHandler {
public:
    GetDeviceInfoHandler(MessageTransport& transport, std::span<const uint8_t> firmware_version);

    [[nodiscard]] bool try_handle(const companion_protocol::Message& request) override;

private:
    MessageTransport& transport_;
    companion_protocol::BoundedBytes<companion_protocol::kMaxFirmwareVersionBytes> firmware_version_{};
};

}  // namespace ble_link
