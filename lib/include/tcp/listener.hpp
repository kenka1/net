#pragma once

#include <netdb.h>

#include <expected>
#include <system_error>

#include "../core/fd.hpp"
#include "../ip/resolver.hpp"

namespace net::tcp {

[[nodiscard]]
inline std::expected<core::FD, std::error_code> TcpListen(const char *host,
                                                          const char *service) noexcept {
    auto ai = ip::GetAddrInfo(host, service, AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
    if (!ai) return std::unexpected(ai.error());

    auto listenfd = ip::Resolve(ai->get());
    if (!listenfd) return std::unexpected(listenfd.error());

    return listenfd;
}
}  // namespace net::tcp
