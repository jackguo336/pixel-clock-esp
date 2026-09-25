#include "mock_led_index.hpp"

#include <array>

#include "display/logical_framebuffer.hpp"
#include "led_matrix_map.hpp"

namespace mock_led_index {
namespace {

constexpr std::size_t kCallCapacity = display::LogicalFramebuffer::kPixelCount;

std::array<Call, kCallCapacity> g_calls{};
std::size_t g_call_count{0};

}  // namespace

void reset()
{
    g_calls.fill(Call{});
    g_call_count = 0;
}

std::size_t call_count()
{
    return g_call_count;
}

const Call* call_at(std::size_t index)
{
    if (index >= g_call_count || index >= kCallCapacity) {
        return nullptr;
    }
    return &g_calls[index];
}

std::size_t index_for_call(std::size_t call_index)
{
    return 1000 + call_index;
}

std::size_t record(int32_t x, int32_t y, led_matrix::LedMatrixOrientation orientation, bool serpentine)
{
    const std::size_t call_index = g_call_count;
    if (call_index < kCallCapacity) {
        g_calls[call_index] = Call{
            .x = x,
            .y = y,
            .orientation = orientation,
            .serpentine = serpentine,
        };
    }
    ++g_call_count;
    return index_for_call(call_index);
}

}  // namespace mock_led_index

namespace led_matrix {

std::size_t led_index_for_logical_pixel(int32_t x, int32_t y, LedMatrixOrientation orientation, bool serpentine)
{
    return mock_led_index::record(x, y, orientation, serpentine);
}

}  // namespace led_matrix
