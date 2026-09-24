#include "demo_bitmap_load.hpp"
#include "demo_ticks.hpp"
#include "display/logical_framebuffer.hpp"
#include "file_system/assets_fs.hpp"
#include "led_matrix/led_matrix_output.hpp"
#include "platform/runtime.hpp"

namespace {

constexpr int kStartupMatrixGpio = 3;

}  // namespace

extern "C" void app_main(void)
{
    display::LogicalFramebuffer framebuffer;
    if (file_system::mount_assets() == ESP_OK) {
        app::load_test_bitmap(framebuffer);
    }

    static led_matrix::LedMatrixOutput matrix({
        .gpio_num = kStartupMatrixGpio,
        .orientation = led_matrix::LedMatrixOrientation::Horizontal,
        .serpentine = true,
    });
    if (matrix.initialize() == led_matrix::LedMatrixOutputStatus::Ok) {
        static_cast<void>(matrix.present(framebuffer));
    }

    static demo::TickSource tick_source;
    static demo::TickSink tick_sink(tick_source);

    auto& runtime = platform::Runtime::instance();
    runtime.add(tick_source);
    runtime.add(tick_sink);
    runtime.start();
}
