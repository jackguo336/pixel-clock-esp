#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>

namespace companion_protocol {

// Stores bytes whose length may be shorter than a compile-time maximum.
// Preferred over built in types such as std::array which does not track length, or
// std::vector which requires dynamic allocation.
template <std::size_t Capacity>
class BoundedBytes {
public:
    static_assert(Capacity <= std::numeric_limits<uint8_t>::max(), "capacity must fit in a uint8_t");

    [[nodiscard]] bool assign(std::span<const uint8_t> bytes)
    {
        if (bytes.size() > Capacity) {
            return false;
        }
        if (!bytes.empty()) {
            std::memmove(bytes_.data(), bytes.data(), bytes.size());
        }
        if (bytes.size() < Capacity) {
            std::memset(bytes_.data() + bytes.size(), 0, Capacity - bytes.size());
        }
        length_ = static_cast<uint8_t>(bytes.size());
        return true;
    }

    [[nodiscard]] bool assign(std::string_view text)
    {
        const auto* const data = reinterpret_cast<const uint8_t*>(text.data());
        return assign(std::span<const uint8_t>(data, text.size()));
    }

    [[nodiscard]] std::span<const uint8_t> view() const
    {
        return std::span<const uint8_t>(bytes_.data(), length_);
    }

    [[nodiscard]] std::size_t size() const
    {
        return length_;
    }

    static constexpr std::size_t capacity()
    {
        return Capacity;
    }

private:
    std::array<uint8_t, Capacity> bytes_{};
    uint8_t length_{0};
};

}  // namespace companion_protocol
