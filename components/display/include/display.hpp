#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "bitmap_file.hpp"
#include "color.hpp"
#include "element_tree.hpp"
#include "element_tree_renderer.hpp"
#include "logical_framebuffer.hpp"
#include "led_matrix_output.hpp"
#include "platform/runtime_component.hpp"

namespace display_runtime {

class DisplayRuntime : public platform::RuntimeComponent {
public:
    const char* name() const override;
    void start() override;
    void on_event(const platform::Event& event) override;

private:
    void refresh();

    static constexpr uint16_t kTestBitmapWidth = 2;
    static constexpr uint16_t kTestBitmapHeight = 2;
    static constexpr std::size_t kTestBitmapPixelCount =
        static_cast<std::size_t>(kTestBitmapWidth) * static_cast<std::size_t>(kTestBitmapHeight);

    std::array<display::RgbColor, kTestBitmapPixelCount> pixels_{};
    display::BitmapFile bitmap_{};
    std::array<display::ElementTreeNode, 1> nodes_{};
    display::ElementTree tree_{};
    display::LogicalFramebuffer framebuffer_{};
    display::ElementTreeRenderer renderer_{};
    led_matrix::LedMatrixOutput output_{led_matrix::LedMatrixOutputConfig{}};
    bool scene_ready_{false};
};

}  // namespace display_runtime
