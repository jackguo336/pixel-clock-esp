#include "display_runtime/display_runtime.hpp"
#include "file_system/assets_fs.hpp"
#include "platform/runtime.hpp"

#include <chrono>
#include <cstdint>

#include "esp_log.h"

namespace {

constexpr char kTag[] = "main";
constexpr int kRefreshesPerSecond = 15;
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

    auto& runtime = platform::Runtime::instance();
    static display_runtime::DisplayRuntime display;
    runtime.add(display);

    platform::Event refresh{};
    refresh.type = platform::EventType::RefreshDisplay;
    refresh.source = &display;
    refresh.coalesce_key = kRefreshCoalesceKey;
    (void)runtime.scheduler().every(kRefreshPeriod, refresh);
    runtime.start();
}
