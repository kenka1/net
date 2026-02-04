#include "net.hpp"

net::awaitable<void> Session(tcp::socket socket)
{
  char buf[Config::MAXLINE];

  try {
    for (;;) {
      std::size_t size = co_await socket.async_read_some(net::buffer(buf), net::use_awaitable);
      co_await net::async_write(socket, net::buffer(buf, size), net::use_awaitable);
    }
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

net::awaitable<void> Listener(net::io_context& io, tcp::endpoint ep)
{
  tcp::acceptor acceptor{io, ep};

  for (;;) {
    try {
      auto socket = co_await acceptor.async_accept(net::use_awaitable);
      net::co_spawn(io, Session(std::move(socket)), net::detached);
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
  }
}

int main()
{
  int io_threads = std::max(1, Config::IO_THREADS);
  net::io_context io{io_threads};

  net::signal_set signals(io, SIGINT, SIGTERM);
  signals.async_wait([&io](auto, auto){io.stop();});

  net::co_spawn(io, Listener(io, tcp::endpoint{tcp::v4(), Config::PORT}), net::detached);

  // Run I/O
  std::vector<std::jthread> io_thread_pool(io_threads - 1);
  for (auto i = 0; i < io_threads - 1; i++)
    io_thread_pool.emplace_back([&io]{ io.run(); });
  io.run();
}
