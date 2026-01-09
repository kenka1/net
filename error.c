#include "net.h"

#include <stdarg.h>

static void err_doit(int save_errno, const char* format, va_list ap);

void err_quit(const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  err_doit(0, format, ap);

  va_end(ap);
  exit(EXIT_FAILURE);
}

void err_sys(const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  err_doit(errno, format, ap);

  va_end(ap);
  exit(EXIT_FAILURE);
}

void err_msg(const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  err_doit(errno, format, ap);

  va_end(ap);
}

void err_doit(int save_errno, const char* format, va_list ap)
{
  int n;
  char buf[MAXLINE + 1];

  vsnprintf(buf, MAXLINE, format, ap);

  if (save_errno) {
    n = strlen(buf);
    snprintf(buf + n, MAXLINE - n, " : %s", strerror(save_errno));
  }

  fflush(stdout);
  fputs(buf, stderr);
  fflush(stderr);
}
