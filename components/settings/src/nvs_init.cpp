#include "settings/nvs_init.hpp"

#include "esp_log.h"
#include "nvs_flash.h"

namespace settings {
namespace {

constexpr char kTag[] = "settings";

}  // namespace

esp_err_t initialize_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(kTag, "erase and retry: %s", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(kTag, "nvs erase: %s", esp_err_to_name(err));
            return err;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "nvs init: %s", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}

}  // namespace settings
