#pragma once

#include <cstddef>
#include <cstdint>

#include "led_matrix/led_matrix_output.hpp"

namespace mock_led_index {

struct Call {
    int32_t x{0};
    int32_t y{0};
    led_matrix::LedMatrixOrientation orientation{led_matrix::LedMatrixOrientation::Horizontal};
    bool serpentine{false};
};

void reset();
std::size_t call_count();
const Call* call_at(std::size_t index);
std::size_t index_for_call(std::size_t call_index);
std::size_t record(int32_t x, int32_t y, led_matrix::LedMatrixOrientation orientation, bool serpentine);

}  // namespace mock_led_index
