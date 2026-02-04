#include "../lib/net.h"

int main(int argc, char** argv)
{
  struct sockaddr_in serv_addr;
  int sockfd, rc;
  long number;
  char *res;
  char buf[1440];

  if (argc != 3)
    err_quit("Usage %s <ip> <Number of datagrams sent\n", argv[0]);

  number = strtol(argv[2], &res, 10);
  if (*res || number < 0)
    err_sys("strtoll error, invalid number\n");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  Inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
  serv_addr.sin_port = htons(18123);

  sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

  while (number > 0) {  
    rc = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (rc <= 0) {
      if (errno == EINTR)
        continue;
      else
        err_sys("sendto error\n");
    }
    number--;
  }
  sendto(sockfd, "", 0, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  exit(EXIT_SUCCESS);
}
