#pragma once

#include <atomic>
#include <cstdint>

#include "ble_link/legacy_advertising_packets_builder.hpp"
#include "host/ble_hs.h"

namespace ble_link {

// NimBLE runs these host callbacks one at a time, so they share this state.
struct HostLinkState {
    LegacyAdvertisingPackets packets{};
    uint8_t own_address_type{0};
    // Stage 5's send reads this from the control task.
    std::atomic<uint16_t> connection_handle{BLE_HS_CONN_HANDLE_NONE};
};

extern HostLinkState host_link;

}  // namespace ble_link
