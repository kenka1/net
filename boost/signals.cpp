#include "net.hpp"

int main()
{
  net::io_context io{};

  net::signal_set signals(io, SIGINT, SIGTERM);
  signals.async_wait([&io](auto, auto){ io.stop(); });
}
