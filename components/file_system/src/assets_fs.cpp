#include "file_system/assets_fs.hpp"

#include <cstddef>

#include "esp_littlefs.h"
#include "esp_log.h"

namespace file_system {
namespace {

constexpr char kTag[] = "assets";
constexpr char kAssetsPartitionLabel[] = "assets";
constexpr char kAssetsBasePath[] = "/assets";

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

}  // namespace file_system
