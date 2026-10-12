#pragma once

// Private to companion_protocol sources. Encoders and decoders touch wire
// bytes only through these cursors.

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

class ByteReader {
public:
    explicit ByteReader(std::span<const uint8_t> buffer) : buffer_(buffer) {}

    [[nodiscard]] bool read_u8(uint8_t& value)
    {
        if (remaining() < sizeof(uint8_t)) {
            return false;
        }
        value = buffer_[offset_];
        ++offset_;
        return true;
    }

    [[nodiscard]] bool read_u16(uint16_t& value)
    {
        if (remaining() < sizeof(uint16_t)) {
            return false;
        }
        uint8_t low = 0;
        uint8_t high = 0;
        if (!read_u8(low) || !read_u8(high)) {
            return false;
        }
        value = static_cast<uint16_t>(static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << CHAR_BIT));
        return true;
    }

    [[nodiscard]] bool read_bytes(std::span<uint8_t> destination)
    {
        if (destination.size() > remaining()) {
            return false;
        }
        if (!destination.empty()) {
            std::memcpy(destination.data(), buffer_.data() + offset_, destination.size());
            offset_ += destination.size();
        }
        return true;
    }

    [[nodiscard]] std::size_t remaining() const
    {
        return buffer_.size() - offset_;
    }

    [[nodiscard]] bool empty() const
    {
        return remaining() == 0;
    }

private:
    std::span<const uint8_t> buffer_{};
    std::size_t offset_{0};
};

}  // namespace companion_protocol
