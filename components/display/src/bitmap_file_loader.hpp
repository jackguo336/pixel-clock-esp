#pragma once

#include <cstdint>

#include "display/bitmap.hpp"

namespace display {

enum class BitmapLoadStatus : uint8_t {
    Ok = 0,
    InvalidArgument,
    OpenFailed,
    ReadFailed,
    TruncatedFile,
    UnsupportedFormat,
    SizeMismatch,
};

class BitmapFileLoader final {
public:
    [[nodiscard]] BitmapLoadStatus load(const char* path, MutableBitmapView destination) const;
};

}  // namespace display
