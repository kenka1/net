#include "lib/net.h"

int main(void)
{
  int listenfd, opt, connfd;
  time_t tick;
  char buf[MAXLINE + 1];
  struct sockaddr_in6 serv_addr;

  listenfd = Socket(AF_INET6, SOCK_STREAM, 0);

  opt = 1;
  Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin6_family = AF_INET6;
  serv_addr.sin6_addr = in6addr_any;
  serv_addr.sin6_port =  htons(DAYTIME_PORT);

  Bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  Listen(listenfd, LISTENQ);

  for (;;) {
    connfd = Accept(listenfd, NULL, NULL);

    tick = time(NULL);
    snprintf(buf, MAXLINE, "%s", ctime(&tick));
    Write(connfd, buf, strlen(buf));

    Close(connfd);
  }
}
