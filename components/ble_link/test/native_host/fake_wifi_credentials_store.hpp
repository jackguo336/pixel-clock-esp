#pragma once

#include "settings/wifi_credentials_store.hpp"

class FakeWifiCredentialsStore : public settings::WifiCredentialsStore {
public:
    explicit FakeWifiCredentialsStore(esp_err_t save_result = ESP_OK)
        : save_result_(save_result)
    {
    }

    esp_err_t save(const settings::WifiCredentials& credentials) const override
    {
        ++save_count;
        saved = credentials;
        return save_result_;
    }

    esp_err_t load(settings::WifiCredentials& credentials) const override
    {
        ++load_count;
        (void)credentials;
        return ESP_FAIL;
    }

    esp_err_t clear() const override
    {
        ++clear_count;
        return ESP_FAIL;
    }

    mutable int save_count{0};
    mutable int load_count{0};
    mutable int clear_count{0};
    mutable settings::WifiCredentials saved{};

private:
    esp_err_t save_result_;
};
