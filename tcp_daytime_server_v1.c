#include "net.h"

int main(int argc, char** argv)
{
  int sockfd, opt, s, connfd;
  time_t tick;
  char buf[MAXLINE + 1];
  struct sockaddr_in serv_addr;

  if (argc != 2)
    err_quit("Usage: %s <IP address>\n", argv[0]);

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    err_sys("socket error\n");

  opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    err_sys("setsockopt error SO_REUSEADDR\n");

  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  if ((s = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)) <= 0) {
    if (s == 0)
      err_quit("Invalid IP address\n");
    else
      err_sys("inet_pton error\n");
  }
  serv_addr.sin_port = htons(DAYTIME_PORT);

  if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    err_sys("bind error\n");

  if (listen(sockfd, LISTENQ) == -1)
    err_sys("listen error\n");

  for (;;) {
    connfd = accept(sockfd, NULL, NULL);
    if (connfd == -1)
      err_sys("accept error\n");

    tick = time(NULL);
    snprintf(buf, MAXLINE, "%s", ctime(&tick));
    if (write(connfd, buf, strlen(buf)) == -1)
      err_msg("write error\n");

    if (close(connfd) == -1)
      err_msg("close error\n");
  }
}
