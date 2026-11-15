#include "ble_link/ble_setup.hpp"
#include "display.hpp"
#include "file_system/assets_fs.hpp"
#include "platform/runtime.hpp"
#include "settings/nvs_init.hpp"
#include "settings/nvs_wifi_credentials_store.hpp"

#include <chrono>
#include <cstdint>

#include "esp_log.h"
#include "nvs.h"

namespace {

constexpr char kTag[] = "main";
constexpr int kRefreshesPerSecond = 30;
constexpr auto kRefreshPeriod =
    std::chrono::microseconds{1'000'000 / kRefreshesPerSecond};
constexpr uint16_t kRefreshCoalesceKey = 1;

void log_stored_wifi_credentials()
{
    settings::NvsWifiCredentialsStore wifi_credentials_store;
    settings::WifiCredentials credentials{};
    const esp_err_t loaded = wifi_credentials_store.load(credentials);
    if (loaded == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(kTag, "no wifi credentials stored");
        return;
    }
    if (loaded != ESP_OK) {
        ESP_LOGE(kTag, "load wifi credentials failed: %s", esp_err_to_name(loaded));
        return;
    }
    ESP_LOGI(kTag, "wifi credentials stored, ssid length %u, passphrase length %u",
             static_cast<unsigned>(credentials.ssid_length), static_cast<unsigned>(credentials.passphrase_length));
}

}  // namespace

extern "C" void app_main(void)
{
    const esp_err_t mounted = file_system::mount_assets();
    if (mounted != ESP_OK) {
        ESP_LOGE(kTag, "mount_assets failed: %s", esp_err_to_name(mounted));
    }

    const esp_err_t nvs_ready = settings::initialize_nvs();
    if (nvs_ready != ESP_OK) {
        ESP_LOGE(kTag, "initialize_nvs failed: %s", esp_err_to_name(nvs_ready));
    } else {
        log_stored_wifi_credentials();
    }

    auto& runtime = platform::Runtime::instance();
    static display_runtime::DisplayRuntime display;
    runtime.add(display);

    ble_link::BluetoothSetup bluetooth_setup_result = ble_link::setup_bluetooth();
    runtime.add(bluetooth_setup_result.consumer);

    platform::Event refresh{};
    refresh.type = platform::EventType::RefreshDisplay;
    refresh.source = &display;
    refresh.coalesce_key = kRefreshCoalesceKey;
    (void)runtime.scheduler().every(kRefreshPeriod, refresh);
    runtime.start();

    if (!bluetooth_setup_result.transport.start()) {
        ESP_LOGE(kTag, "BLE companion link did not start");
    }
}
