#pragma once

#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <system_error>

namespace {
inline constexpr std::size_t BUF_SIZE = 256;
}

namespace net::sys {

inline void SigchildHandler(int) noexcept {
    int saved_errno = errno;
    int status;
    char buf[BUF_SIZE];
    while (::waitpid(-1, &status, WNOHANG) > 0) {
        if (WIFEXITED(status)) {
            ::snprintf(buf, sizeof(buf), "Child exited, status=%d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            ::snprintf(buf, sizeof(buf), "Child killed by signal: %d (%s)\n", WTERMSIG(status),
                       ::strsignal(WTERMSIG(status)));
        } else if (WCOREDUMP(status)) {
            ::snprintf(buf, sizeof(buf), "Core dumped\n");
        } else if (WIFSTOPPED(status)) {
            ::snprintf(buf, sizeof(buf), "Child stopped by signal: %d (%s)\n", WTERMSIG(status),
                       ::strsignal(WTERMSIG(status)));
        } else if (WIFCONTINUED(status)) {
            ::snprintf(buf, sizeof(buf), "Child continued\n");
        } else {
            ::snprintf(buf, sizeof(buf), "what happend to this child? (status=%x)\n",
                       static_cast<unsigned int>(status));
        }

        write(::fileno(stderr), buf, ::strlen(buf));
    }
    errno = saved_errno;
}

[[nodiscard]]
inline std::expected<pid_t, std::error_code> Fork() noexcept {
    pid_t res = fork();
    if (res == -1) {
        int saved_errno = errno;
        return std::unexpected(std::error_code(saved_errno, std::generic_category()));
    }
    return res;
}
}  // namespace net::sys
