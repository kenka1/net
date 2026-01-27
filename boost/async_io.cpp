#include "net.hpp"

static void Write(char* buf, std::size_t size);

static net::io_context io{};
static net::posix::stream_descriptor in{io, STDIN_FILENO};
static net::posix::stream_descriptor out{io, STDOUT_FILENO};

static void Read(char* buf, std::size_t size)
{
  printf("Read()\n");
  in.async_read_some(
    net::mutable_buffer(buf, size),
    [=](const boost::system::error_code& ec, std::size_t n)
    {
      printf("read %ld bytes from input\n", n);
      if (ec)
        fprintf(stderr, "read error: %s\n", ec.what().c_str());
      else
        Write(buf, n);
    }
  );

}

static void Write(char* buf, std::size_t size)
{
  printf("Write()\n");
  out.async_write_some(
    net::const_buffer(buf, size),
    [=](const boost::system::error_code& ec, std::size_t n)
    {
      printf("write %ld bytes to console\n", n);
      std::size_t rest = size;
      if (ec) {
        fprintf(stderr, "write error: %s\n", ec.what().c_str());
      } else {
        rest -= n;
        if (rest > 0)
          Write(buf, rest);
        else
          Read(buf, Config::MAXLINE);
      }
    }
  );
}

int main()
{
  char buf[Config::MAXLINE];
  std::jthread t{[]{io.run();}};
  Read(buf, Config::MAXLINE);
}
