#include "ble_link/ble_setup.hpp"

#include <span>

#include "firmware_version_reader.hpp"

#include "ble_link/companion_message_reply_sender.hpp"
#include "ble_link/get_device_info_handler.hpp"
#include "ble_link/inbound_message_handler.hpp"
#include "ble_link/inbound_message_pool.hpp"
#include "ble_link/set_wifi_credentials_handler.hpp"
#include "settings/nvs_wifi_credentials_store.hpp"

namespace ble_link {

BluetoothSetup setup_bluetooth()
{
    static InboundMessagePool pool;
    static settings::NvsWifiCredentialsStore wifi_credentials_store;
    static BleMessageTransport transport(pool);
    static CompanionMessageReplySender reply_sender(transport);

    const std::span<const uint8_t> firmware_version = firmware_version_bytes();
    static GetDeviceInfoHandler get_device_info(reply_sender, firmware_version);
    static SetWifiCredentialsHandler set_wifi_credentials(reply_sender, wifi_credentials_store);
    static InboundMessageHandler* const handlers[] = {&get_device_info, &set_wifi_credentials};
    static CompanionInboundMessageConsumer consumer(pool, handlers, reply_sender);

    return BluetoothSetup{
        .consumer = consumer,
        .transport = transport,
    };
}

}  // namespace ble_link
