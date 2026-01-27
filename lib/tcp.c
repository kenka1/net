#include "net.h"

int listen_socket(const char *addr, const char *port)
{
  struct sockaddr_in serv_addr;
  int listenfd;
  long res;
  char *endptr;
  const int opt = 1;

  listenfd = Socket(AF_INET, SOCK_STREAM, 0);

  Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  if (addr) {
    if (!inet_pton(AF_INET, addr, &serv_addr.sin_addr)) {
      Close(listenfd);
      err_quit("Invalid address\n");
    }
  } else {
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  
  errno = 0;
  res = strtol(port, &endptr, 10);
  if (*endptr || errno || res < 0 || res >= 65535)
    err_sys("Invalid port\n");
  serv_addr.sin_port = htons(res);

  Bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  Listen(listenfd, LISTENQ);

  return listenfd;
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
