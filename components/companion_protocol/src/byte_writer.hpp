#pragma once

// Appends bytes to a fixed buffer and reports whether the whole sequence fit.
// Prefer this over built-in copies and streams: those leave the write position
// and the buffer limit to the caller, or store bytes in memory that grows.

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace companion_protocol {

class ByteWriter {
public:
    explicit ByteWriter(std::span<uint8_t> buffer) : buffer_(buffer) {}

    [[nodiscard]] bool write_u8(uint8_t value)
    {
        if (failed_ || remaining() < sizeof(uint8_t)) {
            failed_ = true;
            return false;
        }
        buffer_[offset_] = value;
        ++offset_;
        return true;
    }

    [[nodiscard]] bool write_u16(uint16_t value)
    {
        if (failed_ || remaining() < sizeof(uint16_t)) {
            failed_ = true;
            return false;
        }
        const uint8_t low = static_cast<uint8_t>(value);
        const uint8_t high = static_cast<uint8_t>(value >> CHAR_BIT);
        return write_u8(low) && write_u8(high);
    }

    [[nodiscard]] bool write_bytes(std::span<const uint8_t> bytes)
    {
        if (failed_ || bytes.size() > remaining()) {
            failed_ = true;
            return false;
        }
        if (!bytes.empty()) {
            std::memcpy(buffer_.data() + offset_, bytes.data(), bytes.size());
            offset_ += bytes.size();
        }
        return true;
    }

    [[nodiscard]] std::size_t written() const
    {
        return offset_;
    }

    [[nodiscard]] bool ok() const
    {
        return !failed_;
    }

private:
    [[nodiscard]] std::size_t remaining() const
    {
        return buffer_.size() - offset_;
    }

    std::span<uint8_t> buffer_{};
    std::size_t offset_{0};
    bool failed_{false};
};

}  // namespace companion_protocol
