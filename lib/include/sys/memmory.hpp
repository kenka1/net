#pragma once

#include <errno.h>
#include <sys/mman.h>

#include <expected>
#include <system_error>

namespace net::sys {

inline std::expected<void *, std::error_code> Mmap(void *addr, size_t len, int prot, int flags,
                                                   int fd, __off_t offset) noexcept {
    void *ptr = ::mmap(addr, len, prot, flags, fd, offset);
    if (ptr == MAP_FAILED) {
        int saved_errno = errno;
        return std::unexpected(std::error_code(saved_errno, std::generic_category()));
    }
    return ptr;
}

inline std::expected<void, std::error_code> munmap(void *addr, size_t len) noexcept {
    if (::munmap(addr, len) == -1) {
        int saved_errno = errno;
        return std::unexpected(std::error_code(saved_errno, std::generic_category()));
    }
    return {};
}
}  // namespace net::sys
