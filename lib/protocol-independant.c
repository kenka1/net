#include "net.h"

static char buf4[INET_ADDRSTRLEN];

char *sock_ntop(struct sockaddr *addr, socklen_t len)
{
  switch (addr->sa_family) {
    case AF_INET:
    {
      struct sockaddr_in *ptr = (struct sockaddr_in*)addr;
      if (inet_ntop(AF_INET, &ptr->sin_addr, buf4, sizeof(buf4)) == NULL)
        return NULL;
      return buf4;
    }
    case AF_INET6:
      break;
    default:
      ;
  }

  errno = EAFNOSUPPORT;
  return NULL;
}
