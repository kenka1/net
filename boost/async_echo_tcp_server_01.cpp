#include "net.hpp"

/* ===================================================== */
/* ====================== SESSION ====================== */
/* ===================================================== */

class Server;

class Session : public std::enable_shared_from_this<Session> {
public:
  explicit Session(Server& server, tcp::socket socket, std::size_t id) :
    server_(server),
    socket_{std::move(socket)},
    id_{id}
  {}

  void Run() { Read(); }
private:
  void Read()
  {
    auto self = shared_from_this();
    socket_.async_read_some(
      net::mutable_buffer{buf_.data(), buf_.size()},
      [self](const boost::system::error_code& ec, std::size_t size)
      {
        if (ec) {
          std::fprintf(stderr, "read error: %s\n", ec.what().c_str());
          self->Close();
        } else {
          self->size_ = size;
          self->Write();
        }
      }
    );
  }

  void Write()
  {
    auto self = shared_from_this();
    socket_.async_write_some(
      net::const_buffer{buf_.data(), size_},
      [self](const boost::system::error_code& ec, std::size_t size)
      {
        if (ec) {
          std::fprintf(stderr, "write error: %s\n", ec.what().c_str());
          self->Close();
        } else {
          self->size_ -= size;
          if (self->size_ > 0)
            self->Write();
          else
            self->Read();
        }
      }
    );
  }

  void Close();

  Server& server_;
  tcp::socket socket_;
  std::size_t id_;
  std::size_t size_{};
  std::array<std::uint8_t, Config::MAXLINE> buf_;
};

/* ===================================================== */
/* ====================== SERVER ======================= */
/* ===================================================== */

class Server {
public:
  explicit Server(net::io_context& io, tcp::endpoint ep) :
    io_{io},
    acceptor_{io, ep}
  {}

  void Run() { Accept(); }

  void CloseSession(std::size_t id)
  {
    std::lock_guard lock{sessions_mutex_};
    sessions_.erase(id);
  }

private:
  void Accept()
  {
    acceptor_.async_accept(
      net::make_strand(io_),
      [this](const boost::system::error_code& ec, tcp::socket socket)
      {
        if (ec) {
          std::fprintf(stderr, "accept error: %s\n", ec.what().c_str());
        } else {
          std::size_t stored_id = id_.fetch_add(1, std::memory_order_acq_rel);
          auto session = std::make_shared<Session>(*this, std::move(socket), stored_id);

          // Add new session
          {
            std::lock_guard lock{sessions_mutex_};
            sessions_[stored_id] = session;
          }

          session->Run();
        }
        Run();
      }
    );
  }

  // Boost data
  net::io_context& io_;
  tcp::acceptor acceptor_;

  // Sessions data
  std::atomic<std::size_t> id_{};
  std::mutex sessions_mutex_{};
  std::unordered_map<std::size_t, std::shared_ptr<Session>> sessions_{};
};


/* ===================================================== */
/* ====================== SESSION ====================== */
/* ===================================================== */

void Session::Close()
{
  server_.CloseSession(id_);
  socket_.close();
}

int main()
{
  int io_threads = std::max(1, Config::IO_THREADS);
  net::io_context io{io_threads};

  net::signal_set signals(io, SIGINT, SIGTERM);
  signals.async_wait([&io](auto, auto){io.stop();});

  // Run server
  Server server{io, tcp::endpoint{tcp::v4(), Config::PORT}};
  server.Run();

  // Run I/O
  std::vector<std::jthread> io_thread_pool(io_threads - 1);
  for (auto i = 0; i < io_threads - 1; i++)
    io_thread_pool.emplace_back([&io]{ io.run(); });
  io.run();
}
