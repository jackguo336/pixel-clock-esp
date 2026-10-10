#include "display.hpp"

#include <string_view>

#include "elements.hpp"
#include "font_manager.hpp"
#include "platform/log.hpp"

namespace display_runtime {
namespace {

constexpr graphics::RgbColor kWhite{.red = 255, .green = 255, .blue = 255};
constexpr graphics::SolidPaint kTextPaint{.color = kWhite};
constexpr std::string_view kHelloText{"HELLO"};

}  // namespace

const char* DisplayRuntime::name() const
{
    return "display";
}

void DisplayRuntime::start()
{
    scene_ready_ = false;

    const graphics::FontLoadStatus font_status = graphics::FontManager::instance().load();
    if (font_status != graphics::FontLoadStatus::Ok) {
        PLATFORM_LOGE(this, "failed to load font status=%u", static_cast<unsigned>(font_status));
        return;
    }

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
    graphics::ElementTreeBuilder builder(nodes_);
    const graphics::Element text_element{
        .id = {},
        .position = {.x = 0, .y = 0},
        .paint = graphics::Paint{kTextPaint},
        .payload = graphics::TextElementPayload{
            .text = kHelloText,
            .font = &graphics::FontManager::instance().font(graphics::FontId::English7x3),
        },
    };
    if (!builder.add_terminal(text_element)) {
        PLATFORM_LOGE(this, "failed to add text element");
        return;
    }
    tree_ = builder.build();

    framebuffer_.clear();
    renderer_.render(tree_, framebuffer_);
    const led_matrix::LedMatrixOutputStatus status = output_.present(framebuffer_);
    if (status != led_matrix::LedMatrixOutputStatus::Ok) {
        PLATFORM_LOGE(this, "present failed status=%u", static_cast<unsigned>(status));
    }
}

}  // namespace display_runtime
