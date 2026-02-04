#include "net.hpp"

class Session : public std::enable_shared_from_this<Session> {
public:
  explicit Session(net::io_context& io, const tcp::endpoint& ep) :
    io_{io},
    fd_in_{io, STDIN_FILENO},
    fd_out_{io, STDOUT_FILENO},
    socket_{io},
    ep_{ep}
  {}

  void Run() { Connect(); }
private:
  void Connect()
  {
    socket_.async_connect(
      ep_,
      [this](const boost::system::error_code& ec)
      {
        if (ec) {
          fprintf(stderr, "connect error: %s\n", ec.what().c_str());
          Close();
        }
        StartRead();
      }
    );
  }

  void StartRead()
  {
    ReadNet();
    ReadIn();
  }

  void ReadNet()
  {
    socket_.async_read_some(
      net::mutable_buffer{user_buf_.data(), user_buf_.size()},
      [this](const boost::system::error_code& ec, std::size_t size)
      {
        if (ec) {
          std::fprintf(stderr, "read error: %s\n", ec.what().c_str());
          Close();
        } else {
          user_size_ = size;
          WriteOut();
        }
      }
    );
  }

  void ReadIn()
  {
    fd_in_.async_read_some(
      net::mutable_buffer{net_buf_.data(), net_buf_.size()},
      [this](const boost::system::error_code& ec, std::size_t size)
      {
        if (ec) {
          std::fprintf(stderr, "read error: %s\n", ec.what().c_str());
          Close();
        } else {
          net_size_ = size;
          WriteNet();
        }
      }
    );
  }

  void WriteNet()
  {
    socket_.async_write_some(
      net::const_buffer{net_buf_.data(), net_size_},
      [this](const boost::system::error_code& ec, std::size_t size)
      {
        if (ec) {
          std::fprintf(stderr, "write error: %s\n", ec.what().c_str());
          Close();
        } else {
          net_size_ -= size;
          if (net_size_ > 0)
            WriteNet();
          else
            ReadIn();
        }
      }
    );
  }

  void WriteOut()
  {
    fd_out_.async_write_some(
      net::const_buffer{user_buf_.data(), user_size_},
      [this](const boost::system::error_code& ec, std::size_t size)
      {
        if (ec) {
          std::fprintf(stderr, "write error: %s\n", ec.what().c_str());
          Close();
        } else {
          user_size_ -= size;
          if (user_size_ > 0)
            WriteOut();
          else
            ReadNet();
        }
      }
    );
  }

  void Close() 
  { 
    socket_.close();
    fd_in_.close();
    fd_out_.close();
  }

  net::io_context& io_;

  net::posix::stream_descriptor fd_in_;
  net::posix::stream_descriptor fd_out_;

  tcp::socket socket_;
  tcp::endpoint ep_;

  std::size_t user_size_{};
  std::size_t net_size_{};
  std::array<std::uint8_t, Config::MAXLINE> user_buf_;
  std::array<std::uint8_t, Config::MAXLINE> net_buf_;
};

int main(int argc, char** argv)
{
  if (argc != 2) {
    std::fprintf(stderr, "Usage %s <ip>\n", argv[0]);
    return EXIT_FAILURE;
  }

  net::io_context io{};

  net::signal_set signals(io, SIGINT, SIGTERM);
  signals.async_wait([&io](auto, auto){io.stop();});

  boost::system::error_code ec;
  auto addr = ip::make_address_v4(argv[1], ec);
  if (ec) {
    std::fprintf(stderr, "Invalid ip: %s\n", ec.what().c_str());
    return EXIT_FAILURE;
  }

  Session session{io, tcp::endpoint{addr, Config::PORT}};
  session.Run();

  // Run I/O
  io.run();
}
