#pragma once

#include <signal.h>

#include <cstring>
#include <expected>
#include <system_error>

namespace net::sys {

inline std::expected<void, std::error_code> SetSignalHandler(int signal,
                                                             void (*handler)(int)) noexcept {
    struct sigaction sa;
    ::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;

    if (::sigaction(signal, &sa, nullptr) == -1) {
        const int saved_errno = errno;
        return std::unexpected(std::error_code(saved_errno, std::generic_category()));
    }

    return {};
}

}  // namespace net::sys
