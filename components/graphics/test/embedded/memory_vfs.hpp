#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <unistd.h>

#include "esp_err.h"

namespace display::test {

class MemoryVfs final {
public:
    static constexpr char kPrefix[] = "/bitmap-test";

    [[nodiscard]] esp_err_t register_fs();
    [[nodiscard]] esp_err_t unregister_fs();

    void set_file(const char* relative_path, std::span<const uint8_t> bytes);
    void set_fail_reads(bool fail_reads);
    void reset_io_state();

    [[nodiscard]] int close_count() const;
    [[nodiscard]] bool has_open_descriptor() const;

private:
    static int open_file(void* ctx, const char* path, int flags, int mode);
    static int close_file(void* ctx, int fd);
    static ssize_t read_file(void* ctx, int fd, void* destination, size_t size);

    const char* relative_path_{nullptr};
    const uint8_t* data_{nullptr};
    std::size_t size_{0};
    bool fail_reads_{false};
    bool open_{false};
    std::size_t offset_{0};
    int close_count_{0};
    bool registered_{false};
};

}  // namespace display::test
