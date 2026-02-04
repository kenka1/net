#include "../lib/net.h"

int main()
{
  struct sockaddr_in addr;
  socklen_t addr_len;
  int listenfd, sockfd;
  char buf[128];
  ssize_t n;

  listenfd = listen_socket(NULL, "10123");

  addr_len = sizeof(addr);
  sockfd = Accept(listenfd, (struct sockaddr*)&addr, &addr_len);

  for (;;) {
    n = readn(sockfd, buf, sizeof(buf));
    if (n <= 0 || n != sizeof(buf))
      err_sys("read");

    sleep(5);

    n = writen(sockfd, buf, sizeof(buf));
    if (n <= 0 || n != sizeof(buf))
      err_sys("write");
  }
}
