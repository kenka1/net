#include "lib/net.h"

int main(void)
{
  int listenfd, opt, connfd;
  time_t tick;
  char buf[MAXLINE + 1];
  struct sockaddr_in serv_addr;

  listenfd = Socket(AF_INET, SOCK_STREAM, 0);

  opt = 1;
  Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(IPPORT_DAYTIME);

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
