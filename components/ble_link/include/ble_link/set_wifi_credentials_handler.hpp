#pragma once

#include "ble_link/companion_message_reply_sender.hpp"
#include "ble_link/inbound_message_handler.hpp"
#include "settings/wifi_credentials_store.hpp"

namespace ble_link {

class SetWifiCredentialsHandler : public InboundMessageHandler {
public:
    SetWifiCredentialsHandler(CompanionMessageReplySender& reply_sender,
                              settings::WifiCredentialsStore& wifi_credentials_store);

    [[nodiscard]] bool try_handle(const companion_protocol::Message& request) override;

private:
    CompanionMessageReplySender& reply_sender_;
    settings::WifiCredentialsStore& wifi_credentials_store_;
};

}  // namespace ble_link
