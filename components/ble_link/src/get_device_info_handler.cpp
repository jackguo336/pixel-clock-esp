#include "ble_link/get_device_info_handler.hpp"

#include <variant>

#include "companion_protocol/get_device_info.hpp"

namespace ble_link {

GetDeviceInfoHandler::GetDeviceInfoHandler(CompanionMessageReplySender& reply_sender,
                                                 std::span<const uint8_t> firmware_version)
    : reply_sender_(reply_sender)
{
    // assign refuses a version longer than the field and leaves the empty value in place.
    (void)firmware_version_.assign(firmware_version);
}

bool GetDeviceInfoHandler::try_handle(const companion_protocol::Message& request)
{
    if (std::get_if<companion_protocol::GetDeviceInfo>(&request.body) == nullptr) {
        return false;
    }

    companion_protocol::DeviceInfo device_info{};
    device_info.protocol_version = companion_protocol::kProtocolVersion;
    device_info.firmware_version = firmware_version_;
    reply_sender_.send_reply(request.header.request_id, companion_protocol::MessageBody{device_info});
    return true;
}

}  // namespace ble_link
