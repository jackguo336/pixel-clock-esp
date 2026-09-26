#pragma once

#include <cstddef>
#include <cstdint>

#include "led_matrix_output.hpp"

namespace led_matrix {

[[nodiscard]] std::size_t led_index_for_logical_pixel(int32_t x, int32_t y,
                                                      LedMatrixOrientation orientation,
                                                      bool serpentine);

}  // namespace led_matrix
