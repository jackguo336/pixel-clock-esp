#pragma once

#include "settings/wifi_credentials_store.hpp"

#include "nvs.h"

namespace settings {

class OpenedNvsHandle {
public:
    OpenedNvsHandle() = default;

    OpenedNvsHandle(const OpenedNvsHandle&) = delete;
    OpenedNvsHandle& operator=(const OpenedNvsHandle&) = delete;

    ~OpenedNvsHandle()
    {
        if (open_) {
            nvs_close(handle_);
        }
    }

    [[nodiscard]] esp_err_t open(nvs_open_mode_t mode)
    {
        const esp_err_t err = nvs_open(kNvsNamespace, mode, &handle_);
        open_ = err == ESP_OK;
        return err;
    }

    [[nodiscard]] nvs_handle_t get() const
    {
        return handle_;
    }

private:
    nvs_handle_t handle_{};
    // nvs_handle_t is an integer and 0 is not a sentinel.
    bool open_{false};
};

}  // namespace settings
