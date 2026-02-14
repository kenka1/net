#include "lib/net.h"

void doit(int sockfd)
{
  ssize_t n;
  char buf[MAXLINE];

  for (;;) {
    n = Read(sockfd, buf, MAXLINE);
    if (n == 0)
      return;

    n = Write(sockfd, buf, MAXLINE);
  }
}

int main(int argc, char **argv)
{
  int listenfd, sockfd;
  pid_t pid;

  if (argc < 2)
    err_quit("Usage %s <(optional)host> <service>\n", argv[0]);
  else if (argc == 3)
      listenfd = listen_socket(argv[1], argv[2]);
  else if (argc == 2)
    listenfd = listen_socket(NULL, argv[1]);

  for (;;) {
    sockfd = Accept(listenfd, NULL, NULL);

    if ((pid = Fork()) == 0) {
      Close(listenfd);
      doit(sockfd);
      Close(sockfd);
      exit(0);
    }

    Close(sockfd);
  }
}
