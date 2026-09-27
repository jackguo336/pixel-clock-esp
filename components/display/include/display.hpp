#pragma once

#include <array>

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

    std::array<display::ElementTreeNode, 1> nodes_{};
    display::ElementTree tree_{};
    display::LogicalFramebuffer framebuffer_{};
    display::ElementTreeRenderer renderer_{};
    led_matrix::LedMatrixOutput output_{led_matrix::LedMatrixOutputConfig{}};
    bool scene_ready_{false};
};

}  // namespace display_runtime
