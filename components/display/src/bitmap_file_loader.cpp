#include "bitmap_file_loader.hpp"
#include "managed_file_descriptor.hpp"

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
constexpr uint32_t kFileHeaderPixelOffsetField = 10;
constexpr uint32_t kDibSizeFieldBytes = 4;
constexpr uint32_t kDibHeightOffset = 4;
constexpr uint32_t kDibPlanesOffset = 8;
constexpr uint32_t kDibBitCountOffset = 10;
constexpr uint32_t kDibCompressionOffset = 12;
constexpr uint32_t kUncompressedRgb = 0;
constexpr uint16_t kRequiredPlanes = 1;
constexpr uint16_t kBitsPerPixel24 = 24;
constexpr uint16_t kBitsPerPixel32 = 32;
constexpr uint32_t kRowAlignmentBytes = 4;
constexpr std::size_t kSkipChunkBytes = 32;
constexpr std::size_t kBlueChannelIndex = 0;
constexpr std::size_t kGreenChannelIndex = 1;
constexpr std::size_t kRedChannelIndex = 2;
constexpr std::size_t kMaxBytesPerPixel = kBitsPerPixel32 / 8u;

[[nodiscard]] uint16_t read_uint16_little_endian(const uint8_t* bytes)
{
    return static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
}

[[nodiscard]] uint32_t read_uint32_little_endian(const uint8_t* bytes)
{
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

[[nodiscard]] int32_t read_int32_little_endian(const uint8_t* bytes)
{
    return static_cast<int32_t>(read_uint32_little_endian(bytes));
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
    uint8_t discard[kSkipChunkBytes];
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
    uint16_t planes{};
    uint16_t bit_count{};
    uint32_t compression{};
};

/** 
 * Bytes of pixel data in one row, with no BMP 4-byte padding.
 */
[[nodiscard]] uint32_t packed_row_bytes(uint32_t width, uint16_t bit_count)
{
    const uint32_t bytes_per_pixel = static_cast<uint32_t>(bit_count) / 8u;
    return width * bytes_per_pixel;
}

/** 
 * On-disk length of one row: packed pixel bytes rounded up to a 4-byte boundary. 
 */
[[nodiscard]] uint32_t padded_row_stride(uint32_t width, uint16_t bit_count)
{
    const uint32_t unpadded = packed_row_bytes(width, bit_count);
    const uint32_t remainder = unpadded % kRowAlignmentBytes;
    if (remainder == 0) {
        return unpadded;
    }
    // Add 1–3 padding bytes so the next row starts on a 4-byte boundary.
    return unpadded + (kRowAlignmentBytes - remainder);
}

[[nodiscard]] bool parse_headers(int fd, BitmapHeader& header, BitmapLoadStatus& status)
{
    // BMP layout: 14-byte file header, then a DIB (device-independent bitmap) header,
    // then optional palette/gap bytes, then pixel data.
    //
    // File header:
    //   signature (2)     must be "BM" (0x4D42)
    //   file size (4)    unused
    //   reserved (4)     unused
    //   pixel offset (4) byte offset from the start of the file to the first pixel
    //
    // DIB header (BITMAPINFOHEADER, 40 bytes):
    //   header size (4)  must be 40
    //   width (4)        pixels; must be positive
    //   height (4)       pixels; negative means rows are stored top-down
    //   planes (2)      must be 1
    //   bit count (2)    bits per pixel; 24 or 32
    //   compression (4)  0 = uncompressed RGB
    //   remaining fields unused (image size, resolution, palette counts)

    uint8_t file_header[kFileHeaderSize];
    if (!read_exactly(fd, file_header, sizeof(file_header), status)) {
        return false;
    }

    if (read_uint16_little_endian(file_header) != kBitmapSignature) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }

    const uint32_t pixel_offset = read_uint32_little_endian(file_header + kFileHeaderPixelOffsetField);

    uint8_t dib_size_bytes[kDibSizeFieldBytes];
    if (!read_exactly(fd, dib_size_bytes, sizeof(dib_size_bytes), status)) {
        return false;
    }

    const uint32_t dib_header_size = read_uint32_little_endian(dib_size_bytes);
    if (dib_header_size != kBitmapInfoHeaderSize) {
        status = BitmapLoadStatus::UnsupportedFormat;
        return false;
    }

    uint8_t dib_header[kBitmapInfoHeaderSize - kDibSizeFieldBytes];
    if (!read_exactly(fd, dib_header, sizeof(dib_header), status)) {
        return false;
    }

    header.pixel_offset = pixel_offset;
    header.width = read_int32_little_endian(dib_header);
    header.height = read_int32_little_endian(dib_header + kDibHeightOffset);
    header.planes = read_uint16_little_endian(dib_header + kDibPlanesOffset);
    header.bit_count = read_uint16_little_endian(dib_header + kDibBitCountOffset);
    header.compression = read_uint32_little_endian(dib_header + kDibCompressionOffset);
    return true;
}

[[nodiscard]] BitmapLoadStatus check_header_configuration_supported(BitmapHeader& header,
                                                                     MutableBitmapView destination)
{
    if (header.planes != kRequiredPlanes) {
        return BitmapLoadStatus::UnsupportedFormat;
    }
    if (header.bit_count != kBitsPerPixel24 && header.bit_count != kBitsPerPixel32) {
        return BitmapLoadStatus::UnsupportedFormat;
    }
    if (header.compression != kUncompressedRgb) {
        return BitmapLoadStatus::UnsupportedFormat;
    }
    if (header.width <= 0 || header.height == 0 || header.height == INT32_MIN) {
        return BitmapLoadStatus::UnsupportedFormat;
    }

    const uint32_t header_bytes = kFileHeaderSize + kBitmapInfoHeaderSize;
    if (header.pixel_offset < header_bytes) {
        return BitmapLoadStatus::UnsupportedFormat;
    }

    header.top_down = header.height < 0;
    if (header.top_down) {
        header.height = -header.height;
    }

    if (header.width != static_cast<int32_t>(destination.size.width) ||
        header.height != static_cast<int32_t>(destination.size.height)) {
        return BitmapLoadStatus::SizeMismatch;
    }
    return BitmapLoadStatus::Ok;
}

[[nodiscard]] bool skip_to_pixel_data(int fd, const BitmapHeader& header, BitmapLoadStatus& status)
{
    // BMP layout header and pixel data layout (sizes in bytes):
    //   [file header 14] [DIB header 40] [optional palette / gap] [pixel data]
    // The two headers have already been consumed. The file header's pixel offset
    // is the absolute start of the pixel array, so the remaining skip is
    // pixel_offset - header.
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
    // Pixel data (uncompressed RGB):
    //   each pixel  B, G, R [, unused]  3 bytes (24-bit) or 4 bytes (32-bit)
    //   each row    header.width number of pixels, then padding so the row's byte count
    //               is a multiple of 4 (discarded; 32-bit rows already align)
    //   row order   bottom-up by default (file row 0 = last visual row);
    //               top-down when DIB height was negative
    const uint32_t width = static_cast<uint32_t>(header.width);
    const uint32_t height = static_cast<uint32_t>(header.height);
    const uint32_t bytes_per_pixel = static_cast<uint32_t>(header.bit_count) / 8u;
    const uint32_t row_padding =
        padded_row_stride(width, header.bit_count) - packed_row_bytes(width, header.bit_count);

    uint8_t pixel_bytes[kMaxBytesPerPixel];
    uint8_t padding_bytes[kRowAlignmentBytes];
    for (uint32_t file_row = 0; file_row < height; ++file_row) {
        const uint32_t destination_row = header.top_down ? file_row : (height - 1u - file_row);
        // Maps row in the file to the corresponding row in the destination bitmap grid.
        RgbColor* row_pixels =
            destination.pixels.data() + static_cast<std::size_t>(destination_row) * static_cast<std::size_t>(width);
        for (uint32_t column = 0; column < width; ++column) {
            if (!read_exactly(fd, pixel_bytes, bytes_per_pixel, status)) {
                return false;
            }
            row_pixels[column] = RgbColor{
                .red = pixel_bytes[kRedChannelIndex],
                .green = pixel_bytes[kGreenChannelIndex],
                .blue = pixel_bytes[kBlueChannelIndex],
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

    ManagedFileDescriptor file{::open(path, O_RDONLY)};
    if (!file.is_open()) {
        return BitmapLoadStatus::OpenFailed;
    }

    BitmapLoadStatus status = BitmapLoadStatus::Ok;
    BitmapHeader header{};
    if (!parse_headers(file.file_descriptor(), header, status)) {
        return status;
    }

    status = check_header_configuration_supported(header, destination);
    if (status != BitmapLoadStatus::Ok) {
        return status;
    }

    if (!skip_to_pixel_data(file.file_descriptor(), header, status)) {
        return status;
    }
    if (!decode_pixels(file.file_descriptor(), header, destination, status)) {
        return status;
    }
    return BitmapLoadStatus::Ok;
}

}  // namespace display
