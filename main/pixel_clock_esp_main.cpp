#include "assets_fs.hpp"
#include "demo_ticks.hpp"
#include "platform/runtime.hpp"

extern "C" void app_main(void)
{
    if (app::mount_assets() == ESP_OK) {
        app::load_test_bitmap();
    }

    static demo::TickSource tick_source;
    static demo::TickSink tick_sink(tick_source);

    auto& runtime = platform::Runtime::instance();
    runtime.add(tick_source);
    runtime.add(tick_sink);
    runtime.start();
}
