#include "bitmap_file_loader.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <climits>
#include <cstddef>
#include <cstdint>

namespace display {
namespace {

constexpr uint16_t kBitmapSignature = 0x4D42;
constexpr uint32_t kBitmapInfoHeaderSize = 40;
constexpr uint32_t kFileHeaderSize = 14;
constexpr uint32_t kUncompressedRgb = 0;
constexpr uint16_t kRequiredPlanes = 1;
constexpr uint16_t kBitsPerPixel24 = 24;
constexpr uint16_t kBitsPerPixel32 = 32;
constexpr uint32_t kRowAlignmentBytes = 4;

class UniqueFd final {
public:
    UniqueFd() = default;

    explicit UniqueFd(int fd) : fd_(fd) {}

    ~UniqueFd()
    {
        close_if_open();
    }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_)
    {
        other.fd_ = -1;
    }

    UniqueFd& operator=(UniqueFd&& other) noexcept
    {
        if (this != &other) {
            close_if_open();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    [[nodiscard]] int get() const
    {
        return fd_;
    }

    [[nodiscard]] bool is_open() const
    {
        return fd_ >= 0;
    }

private:
    void close_if_open()
    {
        if (fd_ >= 0) {
            static_cast<void>(::close(fd_));
            fd_ = -1;
        }
    }

    int fd_{-1};
};

[[nodiscard]] uint16_t read_le16(const uint8_t* bytes)
{
    return static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
}

[[nodiscard]] uint32_t read_le32(const uint8_t* bytes)
{
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

[[nodiscard]] int32_t read_le32s(const uint8_t* bytes)
{
    return static_cast<int32_t>(read_le32(bytes));
}

[[nodiscard]] bool read_exactly(int fd, void* buffer, std::size_t size, BitmapLoadStatus& status)
{
    auto* out = static_cast<uint8_t*>(buffer);
    std::size_t remaining = size;
    while (remaining > 0) {
        const ssize_t bytes_read = ::read(fd, out, remaining);
        if (bytes_read < 0) {
            status = BitmapLoadStatus::ReadFailed;
            return false;
        }
        if (bytes_read == 0) {
            status = BitmapLoadStatus::TruncatedFile;
            return false;
        }
        out += bytes_read;
        remaining -= static_cast<std::size_t>(bytes_read);
    }
    return true;
}

[[nodiscard]] bool skip_exactly(int fd, std::size_t size, BitmapLoadStatus& status)
{
    uint8_t discard[32];
    std::size_t remaining = size;
    while (remaining > 0) {
        const std::size_t chunk = remaining < sizeof(discard) ? remaining : sizeof(discard);
        if (!read_exactly(fd, discard, chunk, status)) {
            return false;
        }
        remaining -= chunk;
    }
    return true;
}

struct BitmapHeader {
    uint32_t pixel_offset{};
    int32_t width{};
    int32_t height{};
    bool top_down{};
    uint16_t bit_count{};
};

[[nodiscard]] uint32_t packed_row_bytes(uint32_t width, uint16_t bit_count)
{
    const uint32_t bytes_per_pixel = static_cast<uint32_t>(bit_count) / 8u;
    return width * bytes_per_pixel;
}

[[nodiscard]] uint32_t padded_row_stride(uint32_t width, uint16_t bit_count)
{
    const uint32_t unpadded = packed_row_bytes(width, bit_count);
    const uint32_t remainder = unpadded % kRowAlignmentBytes;
    if (remainder == 0) {
        return unpadded;
    }
    return unpadded + (kRowAlignmentBytes - remainder);
}

[[nodiscard]] bool parse_headers(int fd, BitmapHeader& header, BitmapLoadStatus& status)
{
    uint8_t file_header[kFileHeaderSize];
    if (!read_exactly(fd, file_header, sizeof(file_header), status)) {
        return false;
    }

    if (read_le16(file_header) != kBitmapSignature) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }

    const uint32_t pixel_offset = read_le32(file_header + 10);

    uint8_t dib_size_bytes[4];
    if (!read_exactly(fd, dib_size_bytes, sizeof(dib_size_bytes), status)) {
        return false;
    }

    const uint32_t dib_header_size = read_le32(dib_size_bytes);
    if (dib_header_size != kBitmapInfoHeaderSize) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }

    uint8_t dib_header[kBitmapInfoHeaderSize - 4];
    if (!read_exactly(fd, dib_header, sizeof(dib_header), status)) {
        return false;
    }

    const int32_t width = read_le32s(dib_header);
    const int32_t height = read_le32s(dib_header + 4);
    const uint16_t planes = read_le16(dib_header + 8);
    const uint16_t bit_count = read_le16(dib_header + 10);
    const uint32_t compression = read_le32(dib_header + 12);

    if (planes != kRequiredPlanes) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }
    if (bit_count != kBitsPerPixel24 && bit_count != kBitsPerPixel32) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }
    if (compression != kUncompressedRgb) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }
    if (width <= 0 || height == 0 || height == INT32_MIN) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }

    const bool top_down = height < 0;
    const int32_t absolute_height = top_down ? -height : height;
    const uint32_t header_bytes = kFileHeaderSize + kBitmapInfoHeaderSize;
    if (pixel_offset < header_bytes) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }

    header.pixel_offset = pixel_offset;
    header.width = width;
    header.height = absolute_height;
    header.top_down = top_down;
    header.bit_count = bit_count;
    return true;
}

[[nodiscard]] bool skip_to_pixel_data(int fd, const BitmapHeader& header, BitmapLoadStatus& status)
{
    const uint32_t header_bytes = kFileHeaderSize + kBitmapInfoHeaderSize;
    const uint32_t skip_bytes = header.pixel_offset - header_bytes;
    if (skip_bytes == 0) {
        return true;
    }
    return skip_exactly(fd, skip_bytes, status);
}

[[nodiscard]] bool decode_pixels(int fd, const BitmapHeader& header, MutableBitmapView destination,
                                 BitmapLoadStatus& status)
{
    const uint32_t width = static_cast<uint32_t>(header.width);
    const uint32_t height = static_cast<uint32_t>(header.height);
    const uint32_t bytes_per_pixel = static_cast<uint32_t>(header.bit_count) / 8u;
    const uint32_t row_padding =
        padded_row_stride(width, header.bit_count) - packed_row_bytes(width, header.bit_count);

    uint8_t pixel_bytes[4];
    uint8_t padding_bytes[4];
    for (uint32_t file_row = 0; file_row < height; ++file_row) {
        const uint32_t destination_row = header.top_down ? file_row : (height - 1u - file_row);
        RgbColor* row_pixels =
            destination.pixels.data() + static_cast<std::size_t>(destination_row) * static_cast<std::size_t>(width);
        for (uint32_t column = 0; column < width; ++column) {
            if (!read_exactly(fd, pixel_bytes, bytes_per_pixel, status)) {
                return false;
            }
            row_pixels[column] = RgbColor{
                .red = pixel_bytes[2],
                .green = pixel_bytes[1],
                .blue = pixel_bytes[0],
            };
        }
        if (row_padding > 0 && !read_exactly(fd, padding_bytes, row_padding, status)) {
            return false;
        }
    }
    return true;
}

}  // namespace

BitmapLoadStatus BitmapFileLoader::load(const char* path, MutableBitmapView destination) const
{
    if (path == nullptr || path[0] == '\0' || !destination.is_valid()) {
        return BitmapLoadStatus::InvalidArgument;
    }

    UniqueFd file{::open(path, O_RDONLY)};
    if (!file.is_open()) {
        return BitmapLoadStatus::OpenFailed;
    }

    BitmapLoadStatus status = BitmapLoadStatus::Ok;
    BitmapHeader header{};
    if (!parse_headers(file.get(), header, status)) {
        return status;
    }

    if (header.width != static_cast<int32_t>(destination.size.width) ||
        header.height != static_cast<int32_t>(destination.size.height)) {
        return BitmapLoadStatus::SizeMismatch;
    }

    if (!skip_to_pixel_data(file.get(), header, status)) {
        return status;
    }
    if (!decode_pixels(file.get(), header, destination, status)) {
        return status;
    }
    return BitmapLoadStatus::Ok;
}

}  // namespace display
