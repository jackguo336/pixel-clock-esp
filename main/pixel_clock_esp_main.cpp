#include "ble_link/ble_setup.hpp"
#include "display.hpp"
#include "file_system/assets_fs.hpp"
#include "platform/runtime.hpp"
#include "settings/nvs_init.hpp"

#include <chrono>
#include <cstdint>

#include "esp_log.h"

namespace {

constexpr char kTag[] = "main";
constexpr int kRefreshesPerSecond = 30;
constexpr auto kRefreshPeriod =
    std::chrono::microseconds{1'000'000 / kRefreshesPerSecond};
constexpr uint16_t kRefreshCoalesceKey = 1;

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
