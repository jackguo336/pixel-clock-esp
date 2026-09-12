#include "memory_vfs.hpp"

#include <cerrno>
#include <cstring>

#include "esp_vfs.h"
#include "esp_vfs_ops.h"

namespace display::test {

esp_err_t MemoryVfs::register_fs()
{
    static const esp_vfs_fs_ops_t ops = {
        .write_p = nullptr,
        .lseek_p = nullptr,
        .read_p = &MemoryVfs::read_file,
        .pread_p = nullptr,
        .pwrite_p = nullptr,
        .open_p = &MemoryVfs::open_file,
        .close_p = &MemoryVfs::close_file,
        .fstat_p = nullptr,
        .fcntl_p = nullptr,
        .ioctl_p = nullptr,
        .fsync_p = nullptr,
#ifdef CONFIG_VFS_SUPPORT_DIR
        .dir = nullptr,
#endif
#ifdef CONFIG_VFS_SUPPORT_TERMIOS
        .termios = nullptr,
#endif
#if CONFIG_VFS_SUPPORT_SELECT
        .select = nullptr,
#endif
    };

    static_cast<void>(esp_vfs_unregister_fs(kPrefix));
    registered_ = false;

    const esp_err_t err =
        esp_vfs_register_fs(kPrefix, &ops, ESP_VFS_FLAG_STATIC | ESP_VFS_FLAG_CONTEXT_PTR, this);
    if (err == ESP_OK) {
        registered_ = true;
    }
    return err;
}

esp_err_t MemoryVfs::unregister_fs()
{
    if (!registered_) {
        return ESP_OK;
    }

    const esp_err_t err = esp_vfs_unregister_fs(kPrefix);
    if (err == ESP_OK) {
        registered_ = false;
    }
    return err;
}

void MemoryVfs::set_file(const char* relative_path, std::span<const uint8_t> bytes)
{
    relative_path_ = relative_path;
    data_ = bytes.data();
    size_ = bytes.size();
    reset_io_state();
}

void MemoryVfs::set_fail_reads(bool fail_reads)
{
    fail_reads_ = fail_reads;
}

void MemoryVfs::reset_io_state()
{
    open_ = false;
    offset_ = 0;
    close_count_ = 0;
}

int MemoryVfs::close_count() const
{
    return close_count_;
}

bool MemoryVfs::has_open_descriptor() const
{
    return open_;
}

int MemoryVfs::open_file(void* ctx, const char* path, int /*flags*/, int /*mode*/)
{
    auto* self = static_cast<MemoryVfs*>(ctx);
    if (self->relative_path_ == nullptr || path == nullptr || std::strcmp(path, self->relative_path_) != 0) {
        errno = ENOENT;
        return -1;
    }
    if (self->open_) {
        errno = EMFILE;
        return -1;
    }
    self->open_ = true;
    self->offset_ = 0;
    return 0;
}

int MemoryVfs::close_file(void* ctx, int fd)
{
    auto* self = static_cast<MemoryVfs*>(ctx);
    if (!self->open_ || fd != 0) {
        errno = EBADF;
        return -1;
    }
    self->open_ = false;
    ++self->close_count_;
    return 0;
}

ssize_t MemoryVfs::read_file(void* ctx, int fd, void* destination, size_t size)
{
    auto* self = static_cast<MemoryVfs*>(ctx);
    if (!self->open_ || fd != 0 || destination == nullptr) {
        errno = EBADF;
        return -1;
    }
    if (self->fail_reads_) {
        errno = EIO;
        return -1;
    }
    if (self->offset_ >= self->size_) {
        return 0;
    }

    const std::size_t remaining = self->size_ - self->offset_;
    const std::size_t to_copy = remaining < size ? remaining : size;
    std::memcpy(destination, self->data_ + self->offset_, to_copy);
    self->offset_ += to_copy;
    return static_cast<ssize_t>(to_copy);
}

}  // namespace display::test
