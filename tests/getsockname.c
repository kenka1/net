#include "../lib/net.h"

int main(int argc, char **argv)
{
  struct sockaddr_in addr;
  int sockfd, res;
  socklen_t addr_len = sizeof(addr);
  char buf[INET_ADDRSTRLEN];

  if (argc != 2)
    err_quit("Usage %s <host>\n", argv[0]);

  sockfd = Socket(AF_INET, SOCK_STREAM, 0);

  if (make_sockaddr_in(&addr, sizeof(addr), argv[1], "12312") == -1) {
    Close(sockfd);
    exit(EXIT_FAILURE);
  }

  Bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));

  res = getsockname(sockfd, (struct sockaddr*)&addr, &addr_len);
  if (res == -1) {
    Close(sockfd);
    err_sys("getsockname error\n");
  }

  if (inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(addr)) == NULL) {
    Close(sockfd);
    err_quit("inet_ntop error\n");
  }

  printf("%s\n", buf);

  Close(sockfd);
  exit(EXIT_SUCCESS);
}
