#include "net.h"

int Socket(int domain, int type, int protocol)
{
  int fd;

  fd = socket(domain, type, protocol);
  if (fd == -1)
    err_sys("socket error");

  return fd;
}

int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
  if (setsockopt(fd, level, optname, optval, optlen) == -1)
    err_sys("setsockopt error");

  return 0;
}

int Bind(int fd, const struct sockaddr *addr, socklen_t len)
{
  if (bind(fd, addr, len) == -1)
    err_sys("bind error\n");

  return 0;
}

int Connect(int fd, const struct sockaddr *addr, socklen_t len)
{
  if (connect(fd, addr, len) == -1)
    err_sys("connect error\n");

  return 0;
}

int Listen(int fd, int n)
{
  if (listen(fd, n) == -1)
    err_sys("listen error\n");

  return 0;
}

int Accept(int fd, struct sockaddr *addr, socklen_t *addr_len)
{
  int sockfd;

  sockfd = accept(fd, addr, addr_len);
  if (sockfd == -1)
    err_sys("connect error");

  return sockfd;
}

int Write(int fd, const void *buf, size_t n)
{
  if (write(fd, buf, n) == -1) {
    err_msg("write error\n");
    return -1;
  }

  return 0;
}

int Close(int fd)
{
  if (close(fd) == -1) {
    err_msg("close error\n");
    return -1;
  }

  return 0;
}
