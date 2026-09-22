#include "demo_bitmap_load.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "display/bitmap_file.hpp"
#include "display/bitmap_file_loader.hpp"
#include "esp_log.h"

namespace app {
namespace {

constexpr char kTag[] = "assets";
constexpr char kTestBitmapPath[] = "/assets/test.bmp";
constexpr uint16_t kTestBitmapWidth = 2;
constexpr uint16_t kTestBitmapHeight = 2;
constexpr std::size_t kTestBitmapPixelCount =
    static_cast<std::size_t>(kTestBitmapWidth) * static_cast<std::size_t>(kTestBitmapHeight);

}  // namespace

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
