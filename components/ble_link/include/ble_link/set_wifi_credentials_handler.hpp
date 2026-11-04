#pragma once

#include "ble_link/inbound_message_handler.hpp"
#include "settings/wifi_credentials_store.hpp"

namespace ble_link {

class SetWifiCredentialsHandler : public InboundMessageHandler {
public:
    SetWifiCredentialsHandler(MessageTransport& transport, settings::WifiCredentialsStore& wifi_credentials);

    [[nodiscard]] bool try_handle(const companion_protocol::Message& request) override;

private:
    MessageTransport& transport_;
    settings::WifiCredentialsStore& wifi_credentials_;
};

}  // namespace ble_link
