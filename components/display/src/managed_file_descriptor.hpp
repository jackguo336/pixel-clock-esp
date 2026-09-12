#pragma once

#include <unistd.h>

namespace display {

class ManagedFileDescriptor final {
public:
    ManagedFileDescriptor() = default;

    explicit ManagedFileDescriptor(int fd) : fd_(fd) {}

    ~ManagedFileDescriptor()
    {
        close_if_open();
    }

    ManagedFileDescriptor(const ManagedFileDescriptor&) = delete;
    ManagedFileDescriptor& operator=(const ManagedFileDescriptor&) = delete;

    ManagedFileDescriptor(ManagedFileDescriptor&& other) noexcept : fd_(other.fd_)
    {
        other.fd_ = -1;
    }

    ManagedFileDescriptor& operator=(ManagedFileDescriptor&& other) noexcept
    {
        if (this != &other) {
            close_if_open();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    [[nodiscard]] int file_descriptor() const
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

}  // namespace display
