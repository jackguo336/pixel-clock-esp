#include "demo_ticks.hpp"
#include "platform/runtime.hpp"

extern "C" void app_main(void)
{
    static demo::TickSource tick_source;
    static demo::TickSink tick_sink(tick_source);

    auto& runtime = platform::Runtime::instance();
    runtime.add(tick_source);
    runtime.add(tick_sink);
    runtime.start();
}
