#pragma once

#include <span>

#include "ble_link/companion_message_reply_sender.hpp"
#include "ble_link/inbound_message_handler.hpp"
#include "companion_protocol/device_info.hpp"

namespace ble_link {

class GetDeviceInfoHandler : public InboundMessageHandler {
public:
    GetDeviceInfoHandler(CompanionMessageReplySender& reply_sender, std::span<const uint8_t> firmware_version);

    [[nodiscard]] bool try_handle(const companion_protocol::Message& request) override;

private:
    CompanionMessageReplySender& reply_sender_;
    companion_protocol::BoundedBytes<companion_protocol::kMaxFirmwareVersionBytes> firmware_version_{};
};

}  // namespace ble_link
