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

void err_quit(const char* format, ...);
void err_sys(const char* format, ...);
void err_msg(const char* format, ...);

#endif
