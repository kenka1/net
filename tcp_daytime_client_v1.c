#include "lib/net.h"

int main(int argc, char** argv)
{
  int sockfd, s, n;
  char buf[MAXLINE + 1];
  struct sockaddr_in serv_addr;

  if (argc != 2)
    err_quit("Usage: %s <IP address>\n", argv[0]);

  sockfd = Socket(AF_INET, SOCK_STREAM, 0);

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  if ((s = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)) <= 0) {
    if (s == 0)
      err_quit("Invalid IP address\n");
    else
      err_sys("inet_pton error\n");
  }
  serv_addr.sin_port = htons(DAYTIME_PORT);

  Connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  while ((n = read(sockfd, buf, MAXLINE)) > 0) {
    buf[n] = '\0';
    if (fputs(buf, stdout) == EOF)
      err_quit("fputs error");
  }
  if (n < 0)
    err_sys("read error");

  exit(EXIT_SUCCESS);
}
