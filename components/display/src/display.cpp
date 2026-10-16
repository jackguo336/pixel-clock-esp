#include "display.hpp"

#include <chrono>
#include <string_view>

#include "elements.hpp"
#include "font_manager.hpp"
#include "platform/log.hpp"
#include "platform/runtime.hpp"
#include "views/letter_carousel_view.hpp"

namespace display_runtime {
namespace {

constexpr graphics::LinearGradientPaint kTextPaint{
    .angle_degrees = 45.0f,
    .color_stop_count = 5,
    .color_stops =
        {
            graphics::GradientColorStop{.offset = 0.0f, .color = {.red = 255, .green = 0, .blue = 0}},
            graphics::GradientColorStop{.offset = 0.25f, .color = {.red = 255, .green = 127, .blue = 0}},
            graphics::GradientColorStop{.offset = 0.5f, .color = {.red = 255, .green = 255, .blue = 0}},
            graphics::GradientColorStop{.offset = 0.75f, .color = {.red = 0, .green = 255, .blue = 0}},
            graphics::GradientColorStop{.offset = 1.0f, .color = {.red = 0, .green = 190, .blue = 211}},
        },
};
constexpr std::string_view kHelloText{"HELLO"};
constexpr const std::string_view kCarouselTexts[]{"A", "B", "C", "D"};
// One column between HELLO and the carousel. Height 0 lets the row height come from the letters.
constexpr graphics::Size kSpacerSize{.width = 1, .height = 0};
// 3x7 glyphs sit in an 8px-tall slot so the next letter does not peek into the panel at rest.
constexpr graphics::Size kCarouselSlotSize{.width = 3, .height = 8};

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
    const views::ViewData view_data{
        .timestamp = std::chrono::microseconds{platform::Runtime::instance().now()},
    };
    const graphics::Font* const font =
        &graphics::FontManager::instance().font(graphics::FontId::English7x3);
    const views::LetterCarouselView carousel{kCarouselTexts, font, kCarouselSlotSize};

    graphics::ElementTreeBuilder builder(nodes_);
    const graphics::Element root{
        .paint = graphics::Paint{kTextPaint},
        .payload = graphics::ContainerElementPayload{
            .layout_direction = graphics::StackDirection::LeftToRight,
            .layout_system = graphics::LayoutSystem::Stacked,
        },
    };
    const bool scene_built = builder.add_container(root, [&](auto& children) {
        const graphics::Element hello{
            .payload = graphics::TextElementPayload{
                .text = kHelloText,
                .font = font,
            },
        };
        if (!children.add_terminal(hello)) {
            return;
        }

        const graphics::Element spacer{
            .payload = graphics::ContainerElementPayload{
                .fixed_size = kSpacerSize,
            },
        };
        if (!children.add_container(spacer, [](auto&) {})) {
            return;
        }

        if (!carousel.build(view_data, children)) {
            return;
        }
    });
    if (!scene_built) {
        PLATFORM_LOGE(this, "failed to build scene");
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
