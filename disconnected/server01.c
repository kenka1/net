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

  n = read(sockfd, buf, sizeof(buf));
  if (n <= 0)
    err_sys("read error:\n");

  n = writen(sockfd, buf, n);
  if (n <= 0)
    err_sys("write error:\n");

  Close(sockfd);
  Close(listenfd);
}
