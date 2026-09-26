#pragma once

#include <cstdint>

#include "logical_framebuffer.hpp"

namespace led_matrix {

enum class LedMatrixOrientation : uint8_t {
    Horizontal,
    HorizontalReversed,
    Vertical,
    VerticalReversed,
};

struct LedMatrixOutputConfig {
    int gpio_num{3};
    LedMatrixOrientation orientation{LedMatrixOrientation::Vertical};
    bool serpentine{true};
    // WARNING: 256 WS2812Bs at full brightness can draw tens of amps. Consider power
    // supply current limits before increasing this value. Maximum value of a color
    // channel, from 0 to 255.
    uint8_t brightness{50};
};

enum class LedMatrixOutputStatus : uint8_t {
    Ok,
    NotInitialized,
    DriverError,
};

class LedMatrixOutput final {
public:
    explicit LedMatrixOutput(LedMatrixOutputConfig config);
    ~LedMatrixOutput();
    LedMatrixOutput(const LedMatrixOutput&) = delete;
    LedMatrixOutput& operator=(const LedMatrixOutput&) = delete;

    LedMatrixOutputStatus initialize();
    LedMatrixOutputStatus present(const display::LogicalFramebuffer& framebuffer);

private:
    LedMatrixOutputConfig config_;
    void* strip_{nullptr};
};

}  // namespace led_matrix
