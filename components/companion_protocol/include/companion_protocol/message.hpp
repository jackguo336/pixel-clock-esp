#pragma once

#include <variant>

#include "companion_protocol/ack.hpp"
#include "companion_protocol/device_info.hpp"
#include "companion_protocol/error.hpp"
#include "companion_protocol/get_device_info.hpp"
#include "companion_protocol/message_header.hpp"
#include "companion_protocol/set_wifi_credentials.hpp"

namespace companion_protocol {

using MessageBody = std::variant<std::monostate, GetDeviceInfo, SetWifiCredentials, Ack, DeviceInfo, Error>;

struct Message {
    MessageHeader header{};
    MessageBody body{};
};

}  // namespace companion_protocol
