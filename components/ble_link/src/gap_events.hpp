#pragma once

#include "host/ble_gap.h"

namespace ble_link {

int on_gap_event(ble_gap_event* event, void* unused);

}  // namespace ble_link
