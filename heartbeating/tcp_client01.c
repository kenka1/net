#include "../lib/net.h"
#include "config.h"

int main(int argc, char** argv)
{
  int sockfd, retval, heartbeats;
  fd_set readfds, writefds;
  struct timeval timeout;
  msg_t msg;
  size_t msgn;
  ssize_t n;
  time_t t;

  if (argc != 3)
    err_quit("Usage %s <host> <port>", argv[0]);

  sockfd = connect_to_server(argv[1], argv[2]);

  timeout.tv_sec = T1;
  timeout.tv_usec = 0;
  heartbeats = 0;
  msgn = 0;

  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);

  for (;;) {
    retval = select(sockfd + 1, &readfds, &writefds, NULL, &timeout);

    /* error */
    if (retval == -1) {
      if (errno == EINTR)
        continue;
      else {
        close(sockfd);
        err_sys("select error:\n");
      }
    }

    /* time-out */
    if (retval == 0) {

      if (++heartbeats >= 3) {
        close(sockfd);
        err_quit("Connection is lost\n");
      }

      msg.type_ = htonl(MSG_HEARTBEAT); 

      FD_ZERO(&readfds);
      FD_ZERO(&writefds);
      FD_SET(sockfd, &writefds);

      timeout.tv_sec = T2;
      timeout.tv_usec = 0;

      continue;
    }

    /* read */
    if (FD_ISSET(sockfd, &readfds)) {
      n = read(sockfd, (char*)&msg + msgn, sizeof(msg) - msgn);

      /* error */
      if (n <= 0) {
        /* read again */
        if (errno == EINTR) {
          FD_ZERO(&readfds);
          FD_SET(sockfd, &readfds);
          continue;
        }

        close(sockfd);
        if (n == 0)
          err_quit("Server close connection\n");
        else
          err_sys("read error:\n");
      }

      /* update after read */
      msgn += n;
      heartbeats = 0;

      /* msg handling */
      if (msgn == sizeof(msg)) {
        msgn = 0;
        switch (ntohl(msg.type_)) {
          case MSG_TYPE1:
            break;
          case MSG_TYPE2:
            break;
          case MSG_HEARTBEAT:
            t = time(NULL);
            printf("read heartbeat msg: %s\n", ctime(&t));
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_SET(sockfd, &writefds);
            break;
          default:
            ;
        }
      }
    }

    /* write */
    if (FD_ISSET(sockfd, &writefds)) {
      n = write(sockfd, (char*)&msg + msgn, sizeof(msg));  

      /* error */
      if (n <= 0) {
        /* write again */
        if (errno == EINTR) {
          FD_ZERO(&writefds);
          FD_SET(sockfd, &writefds);
          continue;
        }

        close(sockfd);
        err_sys("write error:\n");
      }

      /* update after write */
      msgn += n;
      if (msgn == sizeof(msg)) {
        t = time(NULL);
        printf("write heartbeat msg: %s\n", ctime(&t));
        msgn = 0;
        FD_ZERO(&writefds);
        FD_ZERO(&writefds);
        FD_SET(sockfd, &readfds);
      }
    }

    timeout.tv_sec = T1;
    timeout.tv_usec = 0;
  }
}
