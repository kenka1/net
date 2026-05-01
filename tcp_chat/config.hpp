#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "../lib/include/net.hpp"

namespace tcp = net::tcp;
namespace sys = net::sys;
namespace core = net::core;
namespace io = net::io;

namespace MSG {
inline constexpr std::size_t BUF = 1024 * 16;  // 16 KB
inline constexpr std::size_t SIZE = 512;
inline constexpr std::size_t NUM = 256;
}  // namespace MSG

namespace CFG {
inline constexpr std::size_t SLEEP_MS = 50;  // 50 ms
inline constexpr std::size_t CLIENTS_LIMIT = 10'000;
inline constexpr std::size_t EPOLL_LIMIT = 512;
}  // namespace CFG

struct Msg {
    std::uint64_t stamp_;
    std::size_t size_;
    char data_[MSG::SIZE];
};

struct Packet {
    explicit Packet(std::string data, std::uint64_t stamp)
        : stamp_(stamp), data_(std::move(data)) {}
    std::uint64_t stamp_;
    std::string data_;
};

inline std::optional<std::size_t> ParseBuffer(const std::string_view str) noexcept {
    std::size_t size = str.find('\n');
    if (size == str.npos) return std::nullopt;
    return size + 1;
}

struct Client {
    explicit Client(core::FD sockfd) noexcept : sockfd_(std::move(sockfd)) {}

    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;
    Client(Client &&) = default;
    Client &operator=(Client &&) = default;

    core::FD sockfd_;
    std::size_t buf_size_{};
    std::uint64_t msg_stamp_{};
    std::array<char, MSG::BUF> buf_;
};

struct EpollClient : Client {
    using Client::Client;
    std::uint64_t new_msg_stamp_{};
    std::size_t sent_size_{};
    std::string sent_buf_;
};
