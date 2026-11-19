#pragma once

#include <atomic>
#include <cstdint>

#include "ble_link/buffered_message_assembler.hpp"
#include "ble_link/inbound_message_pool.hpp"
#include "ble_link/legacy_advertising_packets_builder.hpp"
#include "host/ble_hs.h"

namespace ble_link {

// NimBLE runs these host callbacks one at a time, so they share this state.
struct HostLinkState {
    LegacyAdvertisingPackets packets{};
    uint8_t own_address_type{0};
    // send reads this from the control task.
    std::atomic<uint16_t> connection_handle{BLE_HS_CONN_HANDLE_NONE};
    // setup_bluetooth owns the pool. The host task inserts a finished message.
    InboundMessagePool* inbound_messages{nullptr};
    BufferedMessageAssembler assembler{};
    std::atomic<bool> notifications_enabled{false};
    // NimBLE writes this when the outbound characteristic is registered.
    uint16_t outbound_value_handle{0};
};

extern HostLinkState host_link;

}  // namespace ble_link
