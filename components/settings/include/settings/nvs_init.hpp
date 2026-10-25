#pragma once

#include "esp_err.h"

namespace settings {

/// Initializes the default nvs partition.
/// Erases that partition and retries once when init returns
/// ESP_ERR_NVS_NO_FREE_PAGES or ESP_ERR_NVS_NEW_VERSION_FOUND.
[[nodiscard]] esp_err_t initialize_nvs();

}  // namespace settings
