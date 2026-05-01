#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <expected>
#include <memory>
#include <print>
#include <system_error>

#include "../core/fd.hpp"
#include "../sys/socket.hpp"

namespace {
inline constexpr int LISTENQ = 4096;
}

namespace net::ip {
struct addrinfo_deleter {
    void operator()(addrinfo *ai) const noexcept { ::freeaddrinfo(ai); }
};

using addrinfo_ptr = std::unique_ptr<addrinfo, addrinfo_deleter>;

class gai_error_category : public std::error_category {
  public:
    const char *name() const noexcept override { return "getaddrinfo"; }
    std::string message(int e) const override { return ::gai_strerror(e); }
};

inline const std::error_category &gai_category() {
    static gai_error_category ec;
    return ec;
}

[[nodiscard]]
inline std::expected<addrinfo_ptr, std::error_code> GetAddrInfo(const char *host,
                                                                const char *service,
                                                                int family = AF_UNSPEC,
                                                                int socktype = SOCK_STREAM,
                                                                int flags = 0) noexcept {
    addrinfo hints{};
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = flags;

    addrinfo *result = nullptr;

    int res = ::getaddrinfo(host, service, &hints, &result);
    if (res != 0) {
        if (res == EAI_SYSTEM) {
            int saved_errno = errno;
            return std::unexpected(std::error_code(saved_errno, std::generic_category()));
        }
        return std::unexpected(std::error_code(res, gai_category()));
    }

    return addrinfo_ptr(result);
}

inline void LogTcpData(sockaddr *addr, int family) noexcept {
    switch (family) {
        case AF_INET: {
            sockaddr_in *ptr = reinterpret_cast<sockaddr_in *>(addr);
            char buf[INET_ADDRSTRLEN];
            std::println("Socket start listening on address: {} port: {}",
                         ::inet_ntop(AF_INET, &ptr->sin_addr, buf, sizeof(*ptr)),
                         ::ntohs(ptr->sin_port));
            break;
        }
        case AF_INET6: {
            sockaddr_in6 *ptr = reinterpret_cast<sockaddr_in6 *>(addr);
            char buf[INET6_ADDRSTRLEN];
            std::println("Socket start listening on address: {} port: {}",
                         ::inet_ntop(AF_INET, &ptr->sin6_addr, buf, sizeof(*ptr)),
                         ::ntohs(ptr->sin6_port));
            break;
        }
        default:
            std::println("Family does not supported\n");
    }
}

[[nodiscard]]
inline std::expected<core::FD, std::error_code> Resolve(const addrinfo *ai) noexcept {
    for (auto ptr = ai; ptr != nullptr; ptr = ptr->ai_next) {
        auto listenfd = sys::Socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (!listenfd) continue;

        int opt = 1;
        if (::setsockopt(listenfd->GetFD(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
            continue;

        if (ptr->ai_family == AF_INET6) {
            int off = 0;
            if (::setsockopt(listenfd->GetFD(), IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) == -1)
                continue;
        }

        if (::bind(listenfd->GetFD(), ptr->ai_addr, ptr->ai_addrlen) == -1) continue;

        if (::listen(listenfd->GetFD(), LISTENQ) == -1) continue;

        LogTcpData(ptr->ai_addr, ptr->ai_family);

        return listenfd;
    }

    int saved_errno = errno;
    return std::unexpected(std::error_code(saved_errno, std::generic_category()));
}
}  // namespace net::ip
