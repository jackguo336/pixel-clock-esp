#include "unity.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "bitmap_file_loader.hpp"
#include "display/bitmap.hpp"
#include "display/elements.hpp"
#include "memory_vfs.hpp"

namespace {

constexpr char kRelativePath[] = "/ok.bmp";
constexpr char kFullPath[] = "/bitmap-test/ok.bmp";
constexpr char kMissingPath[] = "/bitmap-test/missing.bmp";
constexpr uint16_t kBitmapSignature = 0x4D42;
constexpr uint32_t kBitmapInfoHeaderSize = 40;
constexpr uint32_t kFileHeaderSize = 14;

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

std::vector<uint8_t> make_24bit_bmp(const display::RgbColor& color)
{
    constexpr uint32_t kPixelOffset = kFileHeaderSize + kBitmapInfoHeaderSize;
    constexpr uint32_t kRowStride = 4;
    constexpr uint32_t kFileSize = kPixelOffset + kRowStride;

    std::vector<uint8_t> bytes;
    append_le16(bytes, kBitmapSignature);
    append_le32(bytes, kFileSize);
    append_le16(bytes, 0);
    append_le16(bytes, 0);
    append_le32(bytes, kPixelOffset);
    append_le32(bytes, kBitmapInfoHeaderSize);
    append_le32(bytes, 1);
    append_le32(bytes, 1);
    append_le16(bytes, 1);
    append_le16(bytes, 24);
    append_le32(bytes, 0);
    append_le32(bytes, kRowStride);
    append_le32(bytes, 0);
    append_le32(bytes, 0);
    append_le32(bytes, 0);
    append_le32(bytes, 0);
    bytes.push_back(color.blue);
    bytes.push_back(color.green);
    bytes.push_back(color.red);
    bytes.push_back(0);
    return bytes;
}

class RegisteredMemoryVfs {
public:
    explicit RegisteredMemoryVfs(display::test::MemoryVfs& vfs) : vfs_(&vfs)
    {
        TEST_ASSERT_EQUAL(ESP_OK, vfs_->register_fs());
    }

    ~RegisteredMemoryVfs()
    {
        static_cast<void>(vfs_->unregister_fs());
    }

    RegisteredMemoryVfs(const RegisteredMemoryVfs&) = delete;
    RegisteredMemoryVfs& operator=(const RegisteredMemoryVfs&) = delete;

private:
    display::test::MemoryVfs* vfs_;
};

}  // namespace

TEST_CASE("loads a valid BMP through the /bitmap-test VFS prefix", "[display][vfs]")
{
    const auto bytes = make_24bit_bmp(display::RgbColor{.red = 12, .green = 34, .blue = 56});
    display::test::MemoryVfs vfs;
    vfs.set_file(kRelativePath, bytes);
    const RegisteredMemoryVfs registered(vfs);

    std::array<display::RgbColor, 1> destination{};
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };
    const display::BitmapFileLoader loader;

    TEST_ASSERT_EQUAL(static_cast<int>(display::BitmapLoadStatus::Ok),
                      static_cast<int>(loader.load(kFullPath, view)));
    TEST_ASSERT_EQUAL_UINT8(12, destination[0].red);
    TEST_ASSERT_EQUAL_UINT8(34, destination[0].green);
    TEST_ASSERT_EQUAL_UINT8(56, destination[0].blue);
    TEST_ASSERT_EQUAL(1, vfs.close_count());
    TEST_ASSERT_FALSE(vfs.has_open_descriptor());
}

TEST_CASE("maps a missing VFS path to OpenFailed", "[display][vfs]")
{
    const auto bytes = make_24bit_bmp(display::RgbColor{.red = 1, .green = 2, .blue = 3});
    display::test::MemoryVfs vfs;
    vfs.set_file(kRelativePath, bytes);
    const RegisteredMemoryVfs registered(vfs);

    std::array<display::RgbColor, 1> destination{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };
    const display::BitmapFileLoader loader;

    TEST_ASSERT_EQUAL(static_cast<int>(display::BitmapLoadStatus::OpenFailed),
                      static_cast<int>(loader.load(kMissingPath, view)));
    TEST_ASSERT_EQUAL(0, vfs.close_count());
    TEST_ASSERT_EQUAL_UINT8(9, destination[0].red);
    TEST_ASSERT_EQUAL_UINT8(8, destination[0].green);
    TEST_ASSERT_EQUAL_UINT8(7, destination[0].blue);
}

TEST_CASE("maps a driver read failure to ReadFailed and closes the descriptor", "[display][vfs]")
{
    const auto bytes = make_24bit_bmp(display::RgbColor{.red = 1, .green = 2, .blue = 3});
    display::test::MemoryVfs vfs;
    vfs.set_file(kRelativePath, bytes);
    vfs.set_fail_reads(true);
    const RegisteredMemoryVfs registered(vfs);

    std::array<display::RgbColor, 1> destination{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };
    const display::BitmapFileLoader loader;

    TEST_ASSERT_EQUAL(static_cast<int>(display::BitmapLoadStatus::ReadFailed),
                      static_cast<int>(loader.load(kFullPath, view)));
    TEST_ASSERT_EQUAL(1, vfs.close_count());
    TEST_ASSERT_FALSE(vfs.has_open_descriptor());
    TEST_ASSERT_EQUAL_UINT8(9, destination[0].red);
}

TEST_CASE("closes the VFS descriptor after truncated input", "[display][vfs]")
{
    auto bytes = make_24bit_bmp(display::RgbColor{.red = 1, .green = 2, .blue = 3});
    bytes.resize(10);
    display::test::MemoryVfs vfs;
    vfs.set_file(kRelativePath, bytes);
    const RegisteredMemoryVfs registered(vfs);

    std::array<display::RgbColor, 1> destination{display::RgbColor{.red = 9, .green = 8, .blue = 7}};
    const display::MutableBitmapView view{
        .size = {.width = 1, .height = 1},
        .pixels = destination,
    };
    const display::BitmapFileLoader loader;

    TEST_ASSERT_EQUAL(static_cast<int>(display::BitmapLoadStatus::TruncatedFile),
                      static_cast<int>(loader.load(kFullPath, view)));
    TEST_ASSERT_EQUAL(1, vfs.close_count());
    TEST_ASSERT_FALSE(vfs.has_open_descriptor());
    TEST_ASSERT_EQUAL_UINT8(9, destination[0].red);
    TEST_ASSERT_EQUAL_UINT8(8, destination[0].green);
    TEST_ASSERT_EQUAL_UINT8(7, destination[0].blue);
}
