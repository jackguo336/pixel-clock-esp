#include "display.hpp"

#include "bitmap_file_loader.hpp"
#include "elements.hpp"
#include "platform/log.hpp"

namespace display_runtime {
namespace {

constexpr char kTestBitmapPath[] = "/assets/test.bmp";

}  // namespace

const char* DisplayRuntime::name() const
{
    return "display";
}

void DisplayRuntime::start()
{
    scene_ready_ = false;

    display::MutableBitmapFile destination{};
    destination.size.width = kTestBitmapWidth;
    destination.size.height = kTestBitmapHeight;
    destination.pixels = pixels_;

    const display::BitmapFileLoader loader;
    const display::BitmapLoadStatus load_status = loader.load(kTestBitmapPath, destination);
    if (load_status != display::BitmapLoadStatus::Ok) {
        PLATFORM_LOGE(this, "failed to load %s status=%u", kTestBitmapPath,
                      static_cast<unsigned>(load_status));
        return;
    }

    bitmap_ = destination.as_read_only();

    const led_matrix::LedMatrixOutputStatus output_status = output_.initialize();
    if (output_status != led_matrix::LedMatrixOutputStatus::Ok) {
        PLATFORM_LOGE(this, "led matrix initialize failed status=%u",
                      static_cast<unsigned>(output_status));
    }

    scene_ready_ = true;
}

void DisplayRuntime::on_event(const platform::Event& event)
{
    if (event.type == platform::EventType::RefreshDisplay && event.source == this && scene_ready_) {
        refresh();
    }
}

void DisplayRuntime::refresh()
{
    display::ElementTreeBuilder builder(nodes_);
    const display::Element bitmap_element{
        .id = {},
        .position = {.x = 0, .y = 0},
        .paint = display::SolidPaint{},
        .payload = display::BitmapElementPayload{.bitmap = &bitmap_},
    };
    if (!builder.add_terminal(bitmap_element)) {
        PLATFORM_LOGE(this, "failed to add bitmap element");
        return;
    }
    tree_ = builder.get_tree();

    framebuffer_.clear();
    renderer_.render(tree_, framebuffer_);
    const led_matrix::LedMatrixOutputStatus status = output_.present(framebuffer_);
    if (status != led_matrix::LedMatrixOutputStatus::Ok) {
        PLATFORM_LOGE(this, "present failed status=%u", static_cast<unsigned>(status));
    }
}

}  // namespace display_runtime
