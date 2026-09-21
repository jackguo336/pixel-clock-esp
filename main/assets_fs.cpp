#include "assets_fs.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "display/bitmap.hpp"
#include "display/bitmap_file_loader.hpp"
#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_log.h"

namespace app {
namespace {

constexpr char kTag[] = "assets";
constexpr char kAssetsPartitionLabel[] = "assets";
constexpr char kAssetsBasePath[] = "/assets";
constexpr char kTestBitmapPath[] = "/assets/test.bmp";
constexpr uint16_t kTestBitmapWidth = 2;
constexpr uint16_t kTestBitmapHeight = 2;
constexpr std::size_t kTestBitmapPixelCount =
    static_cast<std::size_t>(kTestBitmapWidth) * static_cast<std::size_t>(kTestBitmapHeight);

}  // namespace

esp_err_t mount_assets()
{
    esp_vfs_littlefs_conf_t conf{};
    conf.base_path = kAssetsBasePath;
    conf.partition_label = kAssetsPartitionLabel;
    conf.read_only = true;

    const esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "mount failed: %s", esp_err_to_name(err));
        return err;
    }

    std::size_t total_bytes = 0;
    std::size_t used_bytes = 0;
    const esp_err_t info_err = esp_littlefs_info(kAssetsPartitionLabel, &total_bytes, &used_bytes);
    if (info_err != ESP_OK) {
        ESP_LOGW(kTag, "mounted at %s; size query failed: %s", kAssetsBasePath,
                 esp_err_to_name(info_err));
        return ESP_OK;
    }

    ESP_LOGI(kTag, "mounted %s total=%u used=%u", kAssetsBasePath,
             static_cast<unsigned>(total_bytes), static_cast<unsigned>(used_bytes));
    return ESP_OK;
}

void load_test_bitmap()
{
    std::array<display::RgbColor, kTestBitmapPixelCount> pixels{};
    display::MutableBitmapFile destination{};
    destination.size.width = kTestBitmapWidth;
    destination.size.height = kTestBitmapHeight;
    destination.pixels = pixels;

    const display::BitmapFileLoader loader;
    const display::BitmapLoadStatus status = loader.load(kTestBitmapPath, destination);
    if (status != display::BitmapLoadStatus::Ok) {
        ESP_LOGE(kTag, "failed to load %s status=%u", kTestBitmapPath,
                 static_cast<unsigned>(status));
        return;
    }

    ESP_LOGI(kTag, "loaded %s %ux%u", kTestBitmapPath, kTestBitmapWidth, kTestBitmapHeight);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        ESP_LOGI(kTag, "pixel[%u] rgb=(%u,%u,%u)", static_cast<unsigned>(i), pixels[i].red,
                 pixels[i].green, pixels[i].blue);
    }
}

}  // namespace app
