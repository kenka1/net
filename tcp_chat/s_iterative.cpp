#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <expected>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <system_error>
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

    std::size_t Size() const noexcept { return msg_stamp_ >= MSG::NUM ? MSG::NUM : msg_stamp_; }

    std::size_t GetStart() const noexcept { return msg_start_; }

    bool Empty() const noexcept { return !msg_stamp_; }

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

static std::expected<void, std::error_code> SendingMessages(Client &client, ServerChat &chat) {
    auto make_res = chat.MakeMsgs(client.msg_stamp_);
    if (!make_res) return {};

    std::string &msgs = make_res->data_;
    std::size_t sent = 0;

    while (sent != msgs.size()) {
        auto write_res = io::Write(client.sockfd_.GetFD(), msgs.data() + sent, msgs.size() - sent);
        if (!write_res) return std::unexpected(write_res.error());
        if (*write_res == 0)
            return std::unexpected(std::error_code(EPIPE, std::generic_category()));

        sent += *write_res;
    }

    client.msg_stamp_ = make_res->stamp_;
    return {};
}

static std::expected<void, std::error_code> HandleClient(Client &client, ServerChat &chat) {
    for (;;) {
        // SEND CLIENT MESSAGES FROM CHAT
        auto send_res = SendingMessages(client, chat);
        if (!send_res) return std::unexpected(send_res.error());

        // READ CLIENT DATA
        auto read_res = io::Read(client.sockfd_.GetFD(), client.buf_.data() + client.buf_size_,
                                 MSG::BUF - client.buf_size_);
        if (!read_res) return std::unexpected(read_res.error());

        if (*read_res == 0) return {};
        client.buf_size_ += *read_res;

        // PARSE DATA
        std::size_t size = chat.ExtractMsgs(client.buf_, client.buf_size_);
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

    // IGNORE SIGPIPE
    if (auto signal_res = sys::SetSignalHandler(SIGPIPE, SIG_IGN); !signal_res) {
        std::println(stderr, "ESetSignalHandl Error: {} {}", signal_res.error().value(),
                     signal_res.error().message());
        return EXIT_FAILURE;
    }

    auto listenfd = tcp::TcpListen(argv[1], argv[2]);
    if (!listenfd) {
        std::println(stderr, "TcpListen Error: {}", listenfd.error().message());
        return EXIT_FAILURE;
    }

    ServerChat chat;

    for (;;) {
        auto sockfd = sys::Accept(listenfd->GetFD(), nullptr, nullptr);
        if (!sockfd) {
            std::println(stderr, "Accept Error: {} {}", sockfd.error().value(),
                         sockfd.error().message());
            continue;
        }

        Client client(std::move(*sockfd));
        try {
            if (auto handle_res = HandleClient(client, chat); !handle_res)
                std::println(stderr, "HandleClient code: {} {}", handle_res.error().value(),
                             handle_res.error().message());
        } catch (const std::exception &e) {
            std::println(stderr, "Threw exception: {}", e.what());
        } catch (...) {
            std::println(stderr, "Threw not std exception");
        }
    }
}
#endif
