#include "net.h"

int make_sockaddr_in(struct sockaddr_in *addr, socklen_t len, const char *ip, const char *port)
{
  long res;
  char *endptr;

  memset(&addr, 0, len);
  addr->sin_family = AF_INET;

  if (!inet_pton(AF_INET, ip, &addr->sin_addr)) {
    fprintf(stderr, "Invalid address\n");
    return -1;
  }
  
  errno = 0;
  res = strtol(port, &endptr, 10);
  if (*endptr || errno || res < 0 || res >= 65535)
    err_sys("Invalid port\n");
  addr->sin_port = htons(res);

  return 0;
}

int listen_socket(const char *host, const char *service)
{
  struct sockaddr_in addr;
  int listenfd;
  long res;
  char *endptr;
  const int opt = 1;

  listenfd = Socket(AF_INET, SOCK_STREAM, 0);

  Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  make_sockaddr_in(&addr, sizeof(addr), host, service);

  Bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));

  Listen(listenfd, LISTENQ);

  return listenfd;
}

int connect_to_server(const char *host, const char *service)
{
  struct sockaddr_in addr;
  int sockfd;

  sockfd = Socket(AF_INET, SOCK_STREAM, 0);

  make_sockaddr_in(&addr, sizeof(addr), host, service);

  Connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));

  return sockfd;
}

ssize_t readn(int fd, void *buf, size_t size)
{
  ssize_t n, rest = size;
  char *ptr = (char*)buf;

  while (rest > 0) {
    n = read(fd, ptr, rest);
    if (n <= 0) {
      /* eof */
      if (n == 0)
        return size - rest;
      
      /* error */
      if (errno == EINTR)
        continue;
      else
        return -1;
    }

    ptr += n;
    rest -= n;
  }

  return size;
}

ssize_t read_size(int fd, void *buf, size_t size)
{
  uint32_t len;
  ssize_t rc;

  rc = readn(fd, &len, sizeof(uint32_t));
  if (rc != sizeof(uint32_t))
    return rc < 0 ? -1 : 0;
  len = ntohl(len);

  if (len > size) {
    /* The msg is too long
     * Flush tcp-buffer 
     * and return error code
     * */

    while (len > 0) {
      rc = readn(fd, buf, size);
      if (rc != size)
        return rc < 0 ? -1 : 0;
      len -= size;
      if (len < size)
        size = len;
    }
    errno = EMSGSIZE;
    return -1;
  }

  rc = readn(fd, buf, len);
  if (rc != len)
    return rc < 0 ? -1 : 0;
  return rc;
}

ssize_t writen(int fd, void *buf, size_t size)
{
  ssize_t n, rest = size;
  char *ptr = (char*)buf;

  while (rest > 0) {
    n = write(fd, buf, rest);
    if (n <= 0) {
      /* eof */
      if (n == 0)
        return size - rest;

      if (errno == EINTR)
        continue;
      else
        return -1;
    }

    ptr += n;
    rest -= n;
  }

  return size;
}
