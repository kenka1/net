#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <expected>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include "config.hpp"

class ServerChat {
  public:
    std::optional<Packet> MakeMsgs(std::uint64_t msg_stamp = 0) {
        const core::SemLockGuard lock(sem_);

        if (!msg_stamp_ || msg_stamp == msg_stamp_) return std::nullopt;

        std::size_t total{};
        std::size_t end = msg_stamp_ > MSG::NUM ? msg_start_ + MSG::NUM : msg_stamp_;
        for (std::size_t i = msg_start_; i < end; i++) {
            if (msgs_[i % MSG::NUM].stamp_ >= msg_stamp) total += msgs_[i % MSG::NUM].size_;
        }

        std::string res;
        res.reserve(total);
        for (std::size_t i = msg_start_; i < end; i++) {
            if (msgs_[i % MSG::NUM].stamp_ >= msg_stamp)
                res.append(msgs_[i % MSG::NUM].data_, msgs_[i % MSG::NUM].size_);
        }

        return Packet{std::move(res), msg_stamp_};
    }

    std::size_t ExtractMsgs(std::span<char> buf, std::size_t size) {
        const core::SemLockGuard lock(sem_);

        std::size_t parse_size = 0;
        while (true) {
            char *tail = buf.data() + parse_size;

            auto parse_res = ParseBuffer({tail, size - parse_size});
            if (!parse_res) return parse_size;

            parse_size += *parse_res;

            // DROP MSG
            if (*parse_res > MSG::SIZE) continue;

            // ADD NEW MSG
            AddMsg(tail, *parse_res);
        }
    }

    std::size_t Size() noexcept {
        const core::SemLockGuard lock(sem_);

        return msg_stamp_ >= MSG::NUM ? MSG::NUM : msg_stamp_;
    }

    std::size_t GetStart() noexcept {
        const core::SemLockGuard lock(sem_);

        return msg_start_;
    }

    bool Empty() noexcept {
        const core::SemLockGuard lock(sem_);

        return !msg_stamp_;
    }

  private:
    void AddMsg(const char *data, std::size_t size) {
        auto &msg = msgs_[msg_stamp_ % MSG::NUM];
        msg.stamp_ = msg_stamp_;
        msg.size_ = size;
        ::memcpy(msg.data_, data, size);

        ++msg_stamp_;
        msg_start_ = msg_stamp_ > MSG::NUM ? msg_stamp_ % MSG::NUM : 0;
    }

    core::Semaphore sem_;
    std::uint64_t msg_stamp_{};
    std::uint64_t msg_start_{};
    Msg msgs_[MSG::NUM];
};

static std::expected<void, std::error_code> SendingMessages(Client &client, ServerChat *chat) {
    auto make_res = chat->MakeMsgs(client.msg_stamp_);
    if (!make_res) return {};

    std::string &msgs = make_res->data_;
    std::size_t sent = 0;

    while (sent != msgs.size()) {
        auto write_res = io::Write(client.sockfd_.GetFD(), msgs.data() + sent, msgs.size() - sent);
        if (!write_res) {
            auto ec = write_res.error();
            if (ec.value() == EWOULDBLOCK || ec.value() == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(CFG::SLEEP_MS));
                continue;
            }
            return std::unexpected(write_res.error());
        }

        sent += *write_res;
    }

    client.msg_stamp_ = make_res->stamp_;
    return {};
}

static void HandleClient(Client client, ServerChat *chat) {
    for (;;) {
        // SEND CLIENT MESSAGES FROM CHAT
        auto send_res = SendingMessages(client, chat);
        if (!send_res) {
            auto ec = send_res.error();
            if (ec.value() == EPIPE || ec.value() == ECONNRESET) exit(EXIT_SUCCESS);

            std::println(stderr, "Error code: {} {}", ec.value(), ec.message());
            exit(EXIT_FAILURE);
        }

        // READ CLIENT DATA
        auto read_res = io::Read(client.sockfd_.GetFD(), client.buf_.data() + client.buf_size_,
                                 MSG::BUF - client.buf_size_);
        if (!read_res) {
            auto ec = read_res.error();
            if (ec.value() == EWOULDBLOCK || ec.value() == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(CFG::SLEEP_MS));
                continue;
            }
            std::println(stderr, "Error code: {} {}", ec.value(), ec.message());
            exit(EXIT_FAILURE);
        }

        if (*read_res == 0) exit(EXIT_SUCCESS);
        client.buf_size_ += *read_res;

        // PARSE DATA
        std::size_t size = chat->ExtractMsgs(client.buf_, client.buf_size_);
        if (size == 0 && client.buf_size_ == MSG::BUF)
            client.buf_size_ = 0;
        else {
            client.buf_size_ -= size;
            std::memmove(client.buf_.data(), client.buf_.data() + size, client.buf_size_);
        }
    }
}

#ifndef TCP_CHAT_TESTING
int main(int argc, char **argv) {
    if (argc != 3) {
        std::println("Usage {} <host> <port>", argv[0]);
        return EXIT_FAILURE;
    }

    // SET SIGCHLD HANDLER
    if (auto signal_res = sys::SetSignalHandler(SIGCHLD, sys::SigchildHandler); !signal_res) {
        std::println(stderr, "Error code: {} {}", signal_res.error().value(),
                     signal_res.error().message());
        return EXIT_FAILURE;
    }

    // IGNORE SIGPIPE
    if (auto signal_res = sys::SetSignalHandler(SIGPIPE, SIG_IGN); !signal_res) {
        std::println(stderr, "Error code: {} {}", signal_res.error().value(),
                     signal_res.error().message());
        return EXIT_FAILURE;
    }

    // OPEN LISTEN SOCKET
    auto listenfd = tcp::TcpListen(argv[1], argv[2]);
    if (!listenfd) {
        std::println(stderr, "Error code: {} {}", listenfd.error().value(),
                     listenfd.error().message());
        return EXIT_FAILURE;
    }

    // ALLOCATE SHARED MEMMORY
    std::size_t size = sizeof(ServerChat);
    auto mmap_res =
        sys::Mmap(nullptr, size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (!mmap_res) {
        std::println(stderr, "Error code: {} {}", mmap_res.error().value(),
                     mmap_res.error().message());
        return EXIT_FAILURE;
    }

    // CONSTRUCT SERVER CHAT IN SHARED MEMMORY
    ServerChat *chat;
    try {
        chat = ::new (*mmap_res) ServerChat();
    } catch (const std::exception &e) {
        std::println(stderr, "Threw exception: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        std::println(stderr, "Threw not std exception");
        return EXIT_FAILURE;
    }

    ::fflush(stdout);

    // ACCEPT LOOP
    for (;;) {
        auto sockfd = sys::Accept(listenfd->GetFD(), nullptr, nullptr);
        if (!sockfd) {
            std::println(stderr, "Accept Error: {} {}", sockfd.error().value(),
                         sockfd.error().message());
            continue;
        }

        if (auto set_res = sys::SetNonblock(sockfd->GetFD()); !set_res) {
            std::println(stderr, "Error code: {} {}", set_res.error().value(),
                         set_res.error().message());
            continue;
        }

        // CLIENT PER FORK
        if (auto fork_res = sys::Fork(); fork_res) {
            if (*fork_res == 0) {
                listenfd->Close();
                HandleClient(Client(std::move(*sockfd)), chat);
            }
        } else {
            std::println(stderr, "Error code: {} {}", fork_res.error().value(),
                         fork_res.error().message());
        }
    }
}
#endif
