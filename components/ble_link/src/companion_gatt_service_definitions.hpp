#pragma once

#include <cstdint>

#include "host/ble_gatt.h"

namespace ble_link {

int refuse_characteristic_access(uint16_t connection_handle,
                                 uint16_t attribute_handle,
                                 ble_gatt_access_ctxt* context,
                                 void* unused);

int on_inbound_write(uint16_t connection_handle,
                     uint16_t attribute_handle,
                     ble_gatt_access_ctxt* context,
                     void* unused);

extern const ble_gatt_svc_def kCompanionServices[];

}  // namespace ble_link
