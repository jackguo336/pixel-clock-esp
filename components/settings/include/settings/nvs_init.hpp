#pragma once

#include "esp_err.h"

namespace settings {

/// Initializes the NVS partition labeled nvs.
[[nodiscard]] esp_err_t initialize_nvs();

}  // namespace settings
