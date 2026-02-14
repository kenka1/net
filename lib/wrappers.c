#include "net.h"

/*===========================*/
/* Wrappers for system calls */
/*===========================*/

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
    err_sys("connect error\n");

  return sockfd;
}

ssize_t Read(int fd, void *buf, size_t count)
{
  ssize_t n;

  n = read(fd, buf, count);
  if (n == -1)
    err_sys("read error\n");

  return n;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
  ssize_t n;

  n = write(fd, buf, count);
  if (n == -1)
    err_sys("write error\n");

  return n;
}

int Close(int fd)
{
  if (close(fd) == -1) {
    err_msg("close error\n");
    return -1;
  }

  return 0;
}

pid_t Fork()
{
  pid_t pid;

  pid = fork();
  if (pid == -1)\
    err_sys("fork error\n");

  return pid;
}

/*==========*/
/* Wrappers */
/*=========+*/

int Inet_ntop(int af, const void *cp, char *buf, socklen_t len)
{
  if (inet_ntop(af, cp, buf, len) == NULL)
    err_sys("inet_ntop error\n");
  return 0;
}

int Inet_pton(int af, const char *cp, void *buf)
{
  int res;

  res = inet_pton(af, cp, buf); 
  if (res <= 0) {
    if (res == 0)
      err_quit("Invalid ip\n");
    else
      err_sys("inet_pton error\n");
  }

  return 0;
}
