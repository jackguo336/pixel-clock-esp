#include "demo_bitmap_load.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "display/bitmap_file.hpp"
#include "display/bitmap_file_loader.hpp"
#include "display/logical_framebuffer.hpp"
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

void load_test_bitmap(display::LogicalFramebuffer& framebuffer)
{
    framebuffer.clear();

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
    for (uint16_t y = 0; y < kTestBitmapHeight; ++y) {
        for (uint16_t x = 0; x < kTestBitmapWidth; ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(kTestBitmapWidth) +
                                      static_cast<std::size_t>(x);
            const display::RgbColor color = pixels[index];
            ESP_LOGI(kTag, "pixel[%u] rgb=(%u,%u,%u)", static_cast<unsigned>(index), color.red, color.green,
                     color.blue);
            if (!framebuffer.set_pixel(x, y, color)) {
                ESP_LOGE(kTag, "framebuffer rejected pixel x=%u y=%u", x, y);
                return;
            }
        }
    }
}

}  // namespace app
