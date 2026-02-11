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
#include <fcntl.h>
#include <sys/select.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096
#define LISTENQ 1024

/*===========================*/
/* Wrappers for system calls */
/*===========================*/

int Socket(int domain, int type, int protocol);
int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
int Bind(int fd, const struct sockaddr *addr, socklen_t len);
int Connect(int fd, const struct sockaddr *addr, socklen_t len);
int Listen(int fd, int n);
int Accept(int fd, struct sockaddr *addr, socklen_t *addr_len);
int Write(int fd, const void *buf, size_t n);
int Close(int fd);

/*==========*/
/* Wrappers */
/*=========+*/

int Inet_ntop(int af, const void *cp, char *buf, socklen_t len);
int Inet_pton(int af, const char *cp, void *buf);

/*=====*/
/* TCP */
/*=====*/

int make_sockaddr_in(struct sockaddr_in *addr, socklen_t len, const char *host, const char *service);
int listen_socket(const char *host, const char *service);

/* Connect to specific host and service
 * return socket or terminate program if error occured
 * */
int connect_to_server(const char *host, const char *service);

/* Read SIZE bytes untill one of the following events occured:
 * 1) SIZE bytes have been read
 * 2) EOF(end of file) have been received
 * 3) An error has occured
 * */
ssize_t readn(int fd, void *buf, size_t size);

/* Read the payload size, 
 * then a buffer equal to this size
 * */
ssize_t read_size(int fd, void *buf, size_t size);

ssize_t writen(int fd, void *buf, size_t size);

/*=====*/
/* UDP */
/*=====*/

int udp_client(const char *host, const char *service);

/*================================*/
/* Protocol-independent functions */
/*================================*/

char* sock_ntop(struct sockaddr *addr, socklen_t len);

/*================*/
/* Error handlers */
/*================*/

void err_quit(const char* format, ...);
void err_sys(const char* format, ...);
void err_msg(const char* format, ...);

#endif
