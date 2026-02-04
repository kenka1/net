#include "../lib/net.h"

int main(void)
{
  struct sockaddr_in serv_addr;
  int sockfd, rc, opt, count = 0;
  char buf[1440];

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(18123);

  sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

  opt = 5000 * 1440;
  Setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));

  Bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  for (;;) {
    rc = recv(sockfd, buf, sizeof(buf), 0);
    if (rc < 0) {
      if (errno == EINTR)
        continue;
      else
        err_sys("sendto error\n");
    }
    count++;
    if (rc == 0)
      break;
  }

  printf("count: %d\n", count);

  exit(EXIT_SUCCESS);
}
