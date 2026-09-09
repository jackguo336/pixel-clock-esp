#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "bitmap_file_loader.hpp"
#include "display/bitmap.hpp"
#include "display/elements.hpp"
#include "gtest/gtest.h"

namespace {

constexpr uint16_t kBitmapSignature = 0x4D42;
constexpr uint32_t kBitmapInfoHeaderSize = 40;
constexpr uint32_t kFileHeaderSize = 14;
constexpr uint8_t kIgnoredAlpha = 0x7A;

void expect_rgb(const display::RgbColor& actual, uint8_t red, uint8_t green, uint8_t blue)
{
    EXPECT_EQ(actual.red, red);
    EXPECT_EQ(actual.green, green);
    EXPECT_EQ(actual.blue, blue);
}

void append_le16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
}

void append_le32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 24));
}

struct BmpFields {
    uint16_t signature = kBitmapSignature;
    uint32_t dib_header_size = kBitmapInfoHeaderSize;
    int32_t width = 1;
    int32_t height = 1;
    uint16_t planes = 1;
    uint16_t bit_count = 24;
    uint32_t compression = 0;
    uint32_t extra_bytes_before_pixels = 0;
    std::vector<display::RgbColor> pixels{};
    std::size_t truncate_to_bytes = 0;
};

[[nodiscard]] uint32_t packed_row_bytes(int32_t width, uint16_t bit_count)
{
    const uint32_t absolute_width = static_cast<uint32_t>(width < 0 ? -width : width);
    return absolute_width * (static_cast<uint32_t>(bit_count) / 8u);
}

[[nodiscard]] uint32_t padded_row_stride(int32_t width, uint16_t bit_count)
{
    const uint32_t unpadded = packed_row_bytes(width, bit_count);
    const uint32_t remainder = unpadded % 4u;
    return remainder == 0 ? unpadded : unpadded + (4u - remainder);
}

[[nodiscard]] std::vector<uint8_t> make_bmp_bytes(const BmpFields& fields)
{
    const int32_t absolute_height = fields.height < 0 ? -fields.height : fields.height;
    const uint32_t stride = padded_row_stride(fields.width, fields.bit_count);
    const uint32_t pixel_data_bytes = stride * static_cast<uint32_t>(absolute_height);
    const uint32_t pixel_offset =
        kFileHeaderSize + fields.dib_header_size + fields.extra_bytes_before_pixels;
    const uint32_t file_size = pixel_offset + pixel_data_bytes;

    std::vector<uint8_t> bytes;
    append_le16(bytes, fields.signature);
    append_le32(bytes, file_size);
    append_le16(bytes, 0);
    append_le16(bytes, 0);
    append_le32(bytes, pixel_offset);
    append_le32(bytes, fields.dib_header_size);

    if (fields.dib_header_size >= kBitmapInfoHeaderSize) {
        append_le32(bytes, static_cast<uint32_t>(fields.width));
        append_le32(bytes, static_cast<uint32_t>(fields.height));
        append_le16(bytes, fields.planes);
        append_le16(bytes, fields.bit_count);
        append_le32(bytes, fields.compression);
        append_le32(bytes, pixel_data_bytes);
        append_le32(bytes, 0);
        append_le32(bytes, 0);
        append_le32(bytes, 0);
        append_le32(bytes, 0);
        bytes.resize(kFileHeaderSize + fields.dib_header_size, 0);
    } else {
        bytes.resize(kFileHeaderSize + fields.dib_header_size, 0);
    }

    bytes.insert(bytes.end(), fields.extra_bytes_before_pixels, 0);

    const uint32_t width = static_cast<uint32_t>(fields.width < 0 ? -fields.width : fields.width);
    const uint32_t bytes_per_pixel = static_cast<uint32_t>(fields.bit_count) / 8u;
    const uint32_t padding = stride - packed_row_bytes(fields.width, fields.bit_count);
    const bool top_down = fields.height < 0;
    for (int32_t file_row = 0; file_row < absolute_height; ++file_row) {
        const int32_t source_row = top_down ? file_row : (absolute_height - 1 - file_row);
        for (uint32_t column = 0; column < width; ++column) {
            const std::size_t index =
                static_cast<std::size_t>(source_row) * static_cast<std::size_t>(width) + column;
            const display::RgbColor color =
                index < fields.pixels.size() ? fields.pixels[index] : display::RgbColor{};
            bytes.push_back(color.blue);
            bytes.push_back(color.green);
            bytes.push_back(color.red);
            if (bytes_per_pixel >= 4) {
                bytes.push_back(kIgnoredAlpha);
            }
        }
        bytes.insert(bytes.end(), padding, 0);
    }

    if (fields.truncate_to_bytes > 0 && fields.truncate_to_bytes < bytes.size()) {
        bytes.resize(fields.truncate_to_bytes);
    }
    return bytes;
}

[[nodiscard]] bool write_file(const std::string& path, const std::vector<uint8_t>& bytes)
{
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return false;
    }
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t written = ::write(fd, bytes.data() + offset, bytes.size() - offset);
        if (written <= 0) {
            static_cast<void>(::close(fd));
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return ::close(fd) == 0;
}

class BitmapFileLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        char tmpl[] = "/tmp/display-bmp-XXXXXX";
        char* directory = ::mkdtemp(tmpl);
        ASSERT_NE(directory, nullptr);
        directory_ = directory;
    }

    void TearDown() override
    {
        for (const std::string& path : files_) {
            static_cast<void>(::unlink(path.c_str()));
        }
        if (!directory_.empty()) {
            static_cast<void>(::rmdir(directory_.c_str()));
        }
    }

    std::string make_path(const char* name)
    {
        std::string path = directory_ + "/" + name;
        files_.push_back(path);
        return path;
    }

    std::string write_bmp(const char* name, const BmpFields& fields)
    {
        const std::string path = make_path(name);
        EXPECT_TRUE(write_file(path, make_bmp_bytes(fields)));
        return path;
    }

    std::string directory_;
    std::vector<std::string> files_;
    display::BitmapFileLoader loader_{};
};

}  // namespace

TEST_F(BitmapFileLoaderTest, DecodesUncompressed24BitBottomUpBmpWithRowPadding)
{
    const std::array<display::RgbColor, 2> expected{
        display::RgbColor{.red = 255, .green = 0, .blue = 0},
        display::RgbColor{.red = 0, .green = 0, .blue = 255},
    };
    const std::string path = write_bmp("padded24.bmp", BmpFields{
        .width = 1,
        .height = 2,
        .bit_count = 24,
        .pixels = {expected[0], expected[1]},
    });
    std::array<display::RgbColor, 2> destination{
        display::RgbColor{.red = 1, .green = 2, .blue = 3},
        display::RgbColor{.red = 4, .green = 5, .blue = 6},
    };
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 2},
        .pixels = destination,
    };

    EXPECT_EQ(loader_.load(path.c_str(), view), display::BitmapLoadStatus::Ok);
    expect_rgb(destination[0], 255, 0, 0);
    expect_rgb(destination[1], 0, 0, 255);
}

TEST_F(BitmapFileLoaderTest, DecodesUncompressed32BitTopDownBmpAndIgnoresAlpha)
{
    const std::array<display::RgbColor, 4> expected{
        display::RgbColor{.red = 10, .green = 20, .blue = 30},
        display::RgbColor{.red = 40, .green = 50, .blue = 60},
        display::RgbColor{.red = 70, .green = 80, .blue = 90},
        display::RgbColor{.red = 100, .green = 110, .blue = 120},
    };
    const std::string path = write_bmp("topdown32.bmp", BmpFields{
        .width = 2,
        .height = -2,
        .bit_count = 32,
        .pixels = {expected[0], expected[1], expected[2], expected[3]},
    });
    std::array<display::RgbColor, 4> destination{};
    const display::MutableBitmapView view{
        .size = {.width = 2, .height = 2},
        .pixels = destination,
    };

    EXPECT_EQ(loader_.load(path.c_str(), view), display::BitmapLoadStatus::Ok);
    expect_rgb(destination[0], 10, 20, 30);
    expect_rgb(destination[1], 40, 50, 60);
    expect_rgb(destination[2], 70, 80, 90);
    expect_rgb(destination[3], 100, 110, 120);
}

TEST_F(BitmapFileLoaderTest, ConvertsBgrChannelsAndPreservesRowOrientation)
{
    const std::string path = write_bmp("rows24.bmp", BmpFields{
        .width = 2,
        .height = 2,
        .bit_count = 24,
        .pixels = {
            display::RgbColor{.red = 255, .green = 0, .blue = 0},
            display::RgbColor{.red = 0, .green = 255, .blue = 0},
            display::RgbColor{.red = 0, .green = 0, .blue = 255},
            display::RgbColor{.red = 255, .green = 255, .blue = 255},
        },
    });
    std::array<display::RgbColor, 4> destination{};
    const display::MutableBitmapView view{
        .size = {.width = 2, .height = 2},
        .pixels = destination,
    };

    EXPECT_EQ(loader_.load(path.c_str(), view), display::BitmapLoadStatus::Ok);
    expect_rgb(destination[0], 255, 0, 0);
    expect_rgb(destination[1], 0, 255, 0);
    expect_rgb(destination[2], 0, 0, 255);
    expect_rgb(destination[3], 255, 255, 255);
}

TEST_F(BitmapFileLoaderTest, RejectsNullEmptyPathsAndInvalidDestinationViews)
{
    std::array<display::RgbColor, 1> destination{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::MutableBitmapView valid{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };
    const display::MutableBitmapView zero_width{
        .size = {.width = 0, .height = 1},
        .pixels = destination,
    };
    const std::string path = write_bmp("valid.bmp", BmpFields{
        .pixels = {display::RgbColor{.red = 1, .green = 2, .blue = 3}},
    });

    EXPECT_EQ(loader_.load(nullptr, valid), display::BitmapLoadStatus::InvalidArgument);
    EXPECT_EQ(loader_.load("", valid), display::BitmapLoadStatus::InvalidArgument);
    EXPECT_EQ(loader_.load(path.c_str(), zero_width), display::BitmapLoadStatus::InvalidArgument);
    expect_rgb(destination[0], 9, 8, 7);
}

TEST_F(BitmapFileLoaderTest, RejectsMissingFiles)
{
    std::array<display::RgbColor, 1> destination{};
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };
    const std::string missing = directory_ + "/missing.bmp";

    EXPECT_EQ(loader_.load(missing.c_str(), view), display::BitmapLoadStatus::OpenFailed);
}

TEST_F(BitmapFileLoaderTest, RejectsBadSignaturesUnsupportedHeadersPlanesDepthsAndCompression)
{
    std::array<display::RgbColor, 1> destination{display::RgbColor{.red = 1, .green = 2, .blue = 3}};
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };

    const std::string bad_signature = write_bmp("bad_sig.bmp", BmpFields{
        .signature = 0x4142,
        .pixels = {display::RgbColor{.red = 9, .green = 9, .blue = 9}},
    });
    const std::string bad_dib = write_bmp("bad_dib.bmp", BmpFields{
        .dib_header_size = 12,
        .pixels = {display::RgbColor{.red = 9, .green = 9, .blue = 9}},
    });
    const std::string bad_planes = write_bmp("bad_planes.bmp", BmpFields{
        .planes = 0,
        .pixels = {display::RgbColor{.red = 9, .green = 9, .blue = 9}},
    });
    const std::string bad_depth = write_bmp("bad_depth.bmp", BmpFields{
        .bit_count = 16,
        .pixels = {display::RgbColor{.red = 9, .green = 9, .blue = 9}},
    });
    const std::string compressed = write_bmp("compressed.bmp", BmpFields{
        .compression = 1,
        .pixels = {display::RgbColor{.red = 9, .green = 9, .blue = 9}},
    });
    const std::string zero_dim = write_bmp("zero.bmp", BmpFields{
        .width = 0,
        .height = 1,
        .pixels = {display::RgbColor{.red = 9, .green = 9, .blue = 9}},
    });

    EXPECT_EQ(loader_.load(bad_signature.c_str(), view), display::BitmapLoadStatus::UnsupportedFormat);
    EXPECT_EQ(loader_.load(bad_dib.c_str(), view), display::BitmapLoadStatus::UnsupportedFormat);
    EXPECT_EQ(loader_.load(bad_planes.c_str(), view), display::BitmapLoadStatus::UnsupportedFormat);
    EXPECT_EQ(loader_.load(bad_depth.c_str(), view), display::BitmapLoadStatus::UnsupportedFormat);
    EXPECT_EQ(loader_.load(compressed.c_str(), view), display::BitmapLoadStatus::UnsupportedFormat);
    EXPECT_EQ(loader_.load(zero_dim.c_str(), view), display::BitmapLoadStatus::UnsupportedFormat);
    expect_rgb(destination[0], 1, 2, 3);
}

TEST_F(BitmapFileLoaderTest, RejectsSourceDimensionsThatDifferFromDestination)
{
    std::array<display::RgbColor, 1> destination{display::RgbColor{.red = 4, .green = 5, .blue = 6}};
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };
    const std::string path = write_bmp("mismatch.bmp", BmpFields{
        .width = 2,
        .height = 1,
        .pixels = {
            display::RgbColor{.red = 9, .green = 9, .blue = 9},
            display::RgbColor{.red = 8, .green = 8, .blue = 8},
        },
    });

    EXPECT_EQ(loader_.load(path.c_str(), view), display::BitmapLoadStatus::SizeMismatch);
    expect_rgb(destination[0], 4, 5, 6);
}

TEST_F(BitmapFileLoaderTest, DistinguishesTruncatedHeadersAndPixelData)
{
    std::array<display::RgbColor, 2> destination{
        display::RgbColor{.red = 1, .green = 1, .blue = 1},
        display::RgbColor{.red = 2, .green = 2, .blue = 2},
    };
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 2},
        .pixels = destination,
    };
    const std::string truncated_header = write_bmp("trunc_header.bmp", BmpFields{
        .width = 1,
        .height = 2,
        .pixels = {
            display::RgbColor{.red = 9, .green = 9, .blue = 9},
            display::RgbColor{.red = 8, .green = 8, .blue = 8},
        },
        .truncate_to_bytes = 20,
    });
    const std::string truncated_pixels = write_bmp("trunc_pixels.bmp", BmpFields{
        .width = 1,
        .height = 2,
        .pixels = {
            display::RgbColor{.red = 9, .green = 9, .blue = 9},
            display::RgbColor{.red = 8, .green = 8, .blue = 8},
        },
        .truncate_to_bytes = kFileHeaderSize + kBitmapInfoHeaderSize + 2,
    });

    EXPECT_EQ(loader_.load(truncated_header.c_str(), view), display::BitmapLoadStatus::TruncatedFile);
    expect_rgb(destination[0], 1, 1, 1);
    expect_rgb(destination[1], 2, 2, 2);

    EXPECT_EQ(loader_.load(truncated_pixels.c_str(), view), display::BitmapLoadStatus::TruncatedFile);
}

TEST_F(BitmapFileLoaderTest, PreservesDestinationWhenValidationFailsBeforePixelDecoding)
{
    std::array<display::RgbColor, 1> destination{display::RgbColor{.red = 11, .green = 22, .blue = 33}};
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };
    const std::string path = write_bmp("gap.bmp", BmpFields{
        .extra_bytes_before_pixels = 4,
        .pixels = {display::RgbColor{.red = 255, .green = 0, .blue = 0}},
        .truncate_to_bytes = kFileHeaderSize + kBitmapInfoHeaderSize + 2,
    });

    EXPECT_EQ(loader_.load(path.c_str(), view), display::BitmapLoadStatus::TruncatedFile);
    expect_rgb(destination[0], 11, 22, 33);
}
