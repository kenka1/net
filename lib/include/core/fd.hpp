#pragma once

#include <errno.h>
#include <unistd.h>

#include <utility>

namespace net::core {
class FD {
  public:
    constexpr FD() = default;
    explicit FD(int fd) noexcept : fd_(fd) {}

    FD(const FD &) = delete;
    FD &operator=(const FD &) = delete;

    FD(FD &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    FD &operator=(FD &&other) noexcept {
        if (this == &other) return *this;

        if (fd_ != -1) Release();

        fd_ = std::exchange(other.fd_, -1);

        return *this;
    }

    ~FD() noexcept {
        if (fd_ != -1) Release();
    }

    [[nodiscard]]
    inline int GetFD() const noexcept {
        return fd_;
    }

    void Close() noexcept {
        if (fd_ != -1) Release();
    }

  private:
    void Release() noexcept {
        const int saved_errno = errno;
        ::close(fd_);
        errno = saved_errno;
        fd_ = -1;
    }

    int fd_{-1};
};
}  // namespace net::core
