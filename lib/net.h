#ifndef NET_H_
#define NET_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096
#define LISTENQ 1024

#define DAYTIME_PORT 13

/* Wrappers for system calls */
int Socket(int domain, int type, int protocol);
int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
int Bind(int fd, const struct sockaddr *addr, socklen_t len);
int Connect(int fd, const struct sockaddr *addr, socklen_t len);
int Listen(int fd, int n);
int Accept(int fd, struct sockaddr *addr, socklen_t *addr_len);
int Write(int fd, const void *buf, size_t n);
int Close(int fd);

/* Error handlers */
void err_quit(const char* format, ...);
void err_sys(const char* format, ...);
void err_msg(const char* format, ...);

#endif
