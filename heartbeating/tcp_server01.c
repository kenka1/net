#include "../lib/net.h"
#include "config.h"

static session* init_session()
{
  session* res = malloc(sizeof(session));
  if (!res)
    return NULL;
  res->state_ = ACTIVE;
  res->heartbeat_ = 0;
  res->msgn_ = 0;
  return res;
}

static void reset_session(session *sess)
{
  sess->state_ = ACTIVE;
  sess->heartbeat_ = 0;
  sess->msgn_ = 0;
}

static void set_session_timed_out(session *sess)
{
  sess->state_ = TIMED_OUT;
  sess->heartbeat_ = 1;
  sess->msgn_ = 0;
  sess->msg_.type_ = MSG_HEARTBEAT;
}

static void set_session_closed(session *sess)
{
  sess->state_ = CLOSED;
}

int main(int argc, char** argv)
{
  int listenfd, sockfd, nfds, retval;
  struct sockaddr_in addr;
  socklen_t addr_len;
  fd_set readfds, writefds;
  struct timeval timeout;
  ssize_t n;
  char readflags[FD_SETSIZE];
  char writeflags[FD_SETSIZE];
  session *sessions[FD_SETSIZE];
  session *session;
  time_t t;

  if (argc != 2)
    err_quit("Usage %s <port>", argv[0]);

  listenfd = listen_socket(NULL, argv[1]);

  timeout.tv_sec = T1;
  timeout.tv_usec = 0;

  memset(&readflags, 0, sizeof(readflags));
  memset(&writeflags, 0, sizeof(readflags));
  memset(sessions, 0, sizeof(sessions));

  readflags[listenfd] = 1;

  for (;;) {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    for (size_t i = 0; i < sizeof(readflags); i++) {
      if (readflags[i] == 1) {
        FD_SET(i, &readfds);
        nfds = i;
      }
      if (writeflags[i] == 1) {
        FD_SET(i, &writefds);
        nfds = i;
      }
    }

    retval = select(nfds + 1, &readfds, &writefds, NULL, &timeout);

    /* error */
    if (retval == -1) {
      if (errno == EINTR)
        continue;
      else {
        for (size_t i = 0; i < sizeof(readflags); i++) {
          if (readflags[i] == 1)
            Close(i);
        }
        break;
      }
    }

    /* time-out */
    if (retval == 0) {
      for (size_t i = 0; i < sizeof(readflags); i++) {
        if (readflags[i] == 1 && i != listenfd) {
          session = sessions[i];
          assert(session);
          switch (session->state_) {
            case ACTIVE:
              set_session_timed_out(session);
              break;
            case TIMED_OUT:
              session->heartbeat_++;
              if (session->heartbeat_ >= 3) {
                readflags[i] = 0;
                set_session_closed(session);
                Close(i);
              }
              break;
            case CLOSED:
              assert(!readflags[i]);
              break;
          }
        }
      }

      timeout.tv_sec = T2;
      timeout.tv_usec = 0;

      continue;
    }

    /* accept new client */
    if (FD_ISSET(listenfd, &readfds)) {
      addr_len = sizeof(addr);
      sockfd = Accept(listenfd, (struct sockaddr*)&addr, &addr_len);

      if (sockfd >= FD_SETSIZE) {
        Close(sockfd);
        fprintf(stderr, "Too many connection, close client connection\n");
      } else {
        readflags[sockfd] = 1;
        /* allocate buffer */
        if (sessions[sockfd] == NULL) {
          sessions[sockfd] = init_session();
          if (sessions[sockfd] == NULL) {
            Close(sockfd);
            fprintf(stderr, "Allocate session error, close client connection\n");
          }
        } else {
          if (sessions[sockfd]->state_ == CLOSED)
            reset_session(sessions[sockfd]);
          else {
            Close(sockfd);
            fprintf(stderr, "Reset session error, attemting to reset the existing connection, close client connection\n");
          }
        }
      }
    }

    /* read */
    for (size_t i = 0; i < sizeof(readflags); i++) {
      if (readflags[i] == 1 && i != listenfd && FD_ISSET(i, &readfds)) {
        session = sessions[i];
        n = read(i, (char*)&session->msg_ + session->msgn_, sizeof(session->msg_) - session->msgn_);

        /* error */
        if (n <= 0) {
          if (errno == EINTR)
            continue;

          /* close connection */
          Close(i);
          readflags[i] = 0;
          if (n == 0)
            printf("Client close connection\n");
          else {
            /* TODO we don't need to close the connection with client because
             * it's a server-side error */
            fprintf(stderr, "read error: %s\n", strerror(errno));
          }

          continue;
        }

        /* update client state after read */
        session->state_ = ACTIVE;
        session->heartbeat_ = 0;
        session->msgn_ += n;

        /* msg handling */
        if (session->msgn_ == sizeof(session->msg_)) {
          session->msgn_ = 0;
          switch (ntohl(session->msg_.type_)) {
            case MSG_TYPE1:
              break;
            case MSG_TYPE2:
              break;
            case MSG_HEARTBEAT:
              t = time(NULL);
              printf("read heartbeat msg: %s\n", ctime(&t));
              readflags[i] = 0;
              writeflags[i] = 1;
              break;
            default:
              ;
          }
        }
      }
    }

    /* write */
    for (size_t i = 0; i < sizeof(writeflags); i++) {
      if (writeflags[i] == 1 && FD_ISSET(i, &writefds)) {
        session = sessions[i];
        if (FD_ISSET(i, &writefds)) {
          n = write(i, (char*)&session->msg_ + session->msgn_, sizeof(session->msg_) - session->msgn_);  

          /* error */
          if (n <= 0) {
            if (errno == EINTR)
              continue;

            /* TODO we don't need to close the connection with client because
             * it's a server-side error */
            Close(i);
            fprintf(stderr, "write error:\n");
          }

          /* update client state after write */
          session->msgn_ += n;
          if (session->msgn_ == sizeof(session->msg_)) {
            t = time(NULL);
            printf("write heartbeat msg: %s\n", ctime(&t));
            session->msgn_ = 0;
            readflags[i] = 1;
            writeflags[i] = 0;
          }
        }
      }
    }

    timeout.tv_sec = T1;
    timeout.tv_usec = 0;
  }

  Close(listenfd);
}
