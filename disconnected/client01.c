#include "../lib/net.h"
#include <stdio.h>

int main()
{
  int sockfd;
  char buf[128];
  size_t len;
  ssize_t n;

  sockfd = connect_to_server("127.0.0.1", "10123");

  while (fgets(buf, 128, stdin) != NULL) {
    n = writen(sockfd, buf, strlen(buf));
    if (n <= 0)
      err_sys("write");

    n = readn(sockfd, buf, sizeof(buf));
    if (n <= 0)
      err_sys("read");

    buf[127] = '\0';
    printf("[%s]\n", buf);
  }
}
