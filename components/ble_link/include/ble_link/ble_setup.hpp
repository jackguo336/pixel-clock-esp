#pragma once

#include "ble_link/ble_message_transport.hpp"
#include "ble_link/companion_inbound_message_consumer.hpp"

namespace ble_link {

struct BluetoothSetup {
    CompanionInboundMessageConsumer& consumer;
    BleMessageTransport& transport;
};

/// Builds the session. Call once, before Runtime::start.
[[nodiscard]] BluetoothSetup setup_bluetooth();

}  // namespace ble_link
