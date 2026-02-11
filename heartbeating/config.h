#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stddef.h>

#define MSG_TYPE1     1
#define MSG_TYPE2     2
#define MSG_HEARTBEAT 3

#define MSG_DATA_SIZE 512

typedef struct {
  uint32_t type_;
  char     data_[MSG_DATA_SIZE];
} msg_t;

typedef enum {
  ACTIVE    = 1,
  TIMED_OUT = 2,
  CLOSED    = 3,
} state;

typedef struct {
  state  state_;
  int    heartbeat_;
  size_t msgn_;
  msg_t  msg_;
} session;

#define T1 6; /* Passive waiting time */
#define T2 1; /* Active waiting time */

#endif
