#include <array>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <expected>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>

#include "config.hpp"

class ServerChat {
  public:
    std::optional<Packet> MakeMsgs(std::uint64_t msg_stamp = 0) {
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

    std::uint64_t GetStamp() noexcept { return msg_stamp_; }

  private:
    void AddMsg(const char *data, std::size_t size) {
        auto &msg = msgs_[msg_stamp_ % MSG::NUM];
        msg.stamp_ = msg_stamp_;
        msg.size_ = size;
        ::memcpy(msg.data_, data, size);

        ++msg_stamp_;
        msg_start_ = msg_stamp_ > MSG::NUM ? msg_stamp_ % MSG::NUM : 0;
    }

    std::uint64_t msg_stamp_{};
    std::uint64_t msg_start_{};
    Msg msgs_[MSG::NUM];
};

static std::expected<std::size_t, std::error_code> WriteToClient(EpollClient &client,
                                                                 ServerChat &chat) {
    if (!client.sent_size_) {
        auto make_res = chat.MakeMsgs(client.msg_stamp_);
        if (!make_res) return 0;
        client.sent_buf_ = std::move(make_res->data_);
        client.new_msg_stamp_ = make_res->stamp_;
    }

    auto write_res = io::Write(client.sockfd_.GetFD(), client.sent_buf_.data() + client.sent_size_,
                               client.sent_buf_.size() - client.sent_size_);
    if (!write_res) return std::unexpected(write_res.error());

    client.sent_size_ += *write_res;
    if (client.sent_size_ == client.sent_buf_.size()) {
        client.msg_stamp_ = client.new_msg_stamp_;
        client.sent_size_ = 0;
        client.sent_buf_.clear();
    }

    return *write_res;
}

static std::expected<std::size_t, std::error_code> ReadFromClient(Client &client,
                                                                  ServerChat &chat) {
    auto read_res = io::Read(client.sockfd_.GetFD(), client.buf_.data() + client.buf_size_,
                             MSG::BUF - client.buf_size_);
    if (!read_res) return std::unexpected(read_res.error());

    if (*read_res == 0) return 0;
    client.buf_size_ += *read_res;

    // PARSE DATA
    std::size_t size = chat.ExtractMsgs(client.buf_, client.buf_size_);
    if (size == 0 && client.buf_size_ == MSG::BUF)
        client.buf_size_ = 0;
    else {
        client.buf_size_ -= size;
        std::memmove(client.buf_.data(), client.buf_.data() + size, client.buf_size_);
    }

    return *read_res;
}

#ifndef TCP_CHAT_TESTING
int main(int argc, char **argv) {
    if (argc != 3) {
        std::println("Usage {} <host> <port>", argv[0]);
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

    // EPOLL
    auto epfd_res = sys::EpollCreate();
    if (!epfd_res) {
        std::println(stderr, "Error code: {} {}", epfd_res.error().value(),
                     epfd_res.error().message());
        return EXIT_FAILURE;
    }

    // ADD LISTEN SOCKET TO EPOLL
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd->GetFD();
    auto event_ctl_res = sys::EpollCtl(epfd_res->GetFD(), EPOLL_CTL_ADD, listenfd->GetFD(), &ev);
    if (!event_ctl_res) {
        std::println(stderr, "EPOLL_CTL_ADD Error: {} {}", event_ctl_res.error().value(),
                     event_ctl_res.error().message());
        return EXIT_FAILURE;
    }

    ServerChat chat;
    std::unordered_map<int, EpollClient> clients_;
    epoll_event evlist[CFG::EPOLL_LIMIT];
    uint64_t prev_stamp_;
    std::deque<int> clients_to_close;

    for (;;) {
        auto wait_res = sys::EpollWait(epfd_res->GetFD(), evlist, CFG::EPOLL_LIMIT, -1);
        if (!wait_res) {
            std::println(stderr, "Epoll wait Error: {} {}", wait_res.error().value(),
                         wait_res.error().message());
            return EXIT_FAILURE;
        }

        prev_stamp_ = chat.GetStamp();
        for (int i = 0; i < *wait_res; ++i) {
            int fd = evlist[i].data.fd;

            // ACCEPT
            if (fd == listenfd->GetFD()) {
                auto sockfd = sys::Accept(listenfd->GetFD(), nullptr, nullptr);
                if (!sockfd) {
                    std::println(stderr, "Accept Error: {} {}", sockfd.error().value(),
                                 sockfd.error().message());
                    continue;
                }

                if (auto set_res = sys::SetNonblock(sockfd->GetFD()); !set_res) {
                    std::println(stderr, "SetNonblock Error: {} {}", set_res.error().value(),
                                 set_res.error().message());
                    continue;
                }

                ev.events = EPOLLIN | EPOLLRDHUP;
                ev.data.fd = sockfd->GetFD();

                auto ctl_res =
                    sys::EpollCtl(epfd_res->GetFD(), EPOLL_CTL_ADD, sockfd->GetFD(), &ev);
                if (!ctl_res) {
                    std::println(stderr, "EPOLL_CTL_ADD Error: {} {}", ctl_res.error().value(),
                                 ctl_res.error().message());
                    continue;
                }

                int key = sockfd->GetFD();
                auto [it, done] = clients_.try_emplace(key, EpollClient(std::move(*sockfd)));
                if (!done) std::println(stderr, "Error: Client already exists");

                continue;
            }

            auto it = clients_.find(fd);
            if (it == clients_.end()) continue;
            auto &client = it->second;

            // ERROR
            if (evlist[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                clients_.erase(fd);
                continue;
            }

            // READ
            if (evlist[i].events & EPOLLIN) {
                auto read_res = ReadFromClient(client, chat);
                if (!read_res) {
                    if (read_res.error() != std::errc::resource_unavailable_try_again) {
                        std::println(stderr, "Read Error: {} {}", read_res.error().value(),
                                     read_res.error().message());
                        clients_.erase(fd);
                        continue;
                    }
                } else if (*read_res == 0) {
                    std::println("Client close connection");
                    clients_.erase(fd);
                    continue;
                }
            }

            // WRITE
            if (evlist[i].events & EPOLLOUT) {
                auto write_res = WriteToClient(client, chat);
                if (!write_res) {
                    if (write_res.error() != std::errc::resource_unavailable_try_again) {
                        std::println(stderr, "Write Error: {} {}", write_res.error().value(),
                                     write_res.error().message());
                        clients_.erase(fd);
                        continue;
                    }
                } else if (*write_res == 0) {
                    ev.events = EPOLLIN | EPOLLRDHUP;
                    ev.data.fd = fd;
                    auto ctl_res = sys::EpollCtl(epfd_res->GetFD(), EPOLL_CTL_MOD, fd, &ev);
                    if (!ctl_res) {
                        std::println(stderr, "EPOLL_CTL_MOD Error: {} {}", ctl_res.error().value(),
                                     ctl_res.error().message());
                        clients_.erase(fd);
                    }
                    continue;
                }
            }
        }

        uint64_t cur_stamp = chat.GetStamp();
        if (prev_stamp_ != cur_stamp) {
            prev_stamp_ = cur_stamp;
            ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;

            for (const auto &[key, client] : clients_) {
                int fd = client.sockfd_.GetFD();
                ev.data.fd = fd;
                auto ctl_res = sys::EpollCtl(epfd_res->GetFD(), EPOLL_CTL_MOD, fd, &ev);
                if (!ctl_res) {
                    std::println(stderr, "EPOLL_CTL_MOD Error: {} {}", ctl_res.error().value(),
                                 ctl_res.error().message());
                    clients_to_close.push_back(fd);
                }
            }

            for (int &fd : clients_to_close) {
                clients_.erase(fd);
            }
            clients_to_close.clear();
        }
    }
}
#endif
