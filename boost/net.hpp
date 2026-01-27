#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

namespace Config
{
  constexpr std::uint16_t PORT  = 13953;
  constexpr std::size_t MAXLINE = 16 * 1024;
  constexpr int IO_THREADS      = 2;
}
