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
      err_sys("write error:\n");

    n = read(sockfd, buf, sizeof(buf));
    if (n <= 0)
      err_sys("read error:\n");

    buf[127] = '\0';
    printf("%s\n", buf);

    printf("sleeping\n");
    sleep(2);

    /* Here we get an error because server already closed the connection */
    printf("try to read\n");
    n = read(sockfd, buf, sizeof(buf));
    if (n <= 0)
      err_sys("read error:\n");

    break;
  }
}
