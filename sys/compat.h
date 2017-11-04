#ifndef _COMPAT_H
#define _COMPAT_H

#include "config.h"
#include <time.h>

#ifdef WIN32
# include "win32/net.h"
#else
# define closesocket(x) close(x)
#endif

#ifndef HAVE_ASPRINTF
int asprintf(char **sptr, const char *fmt, ...);
int vasprintf(char **sptr, const char *fmt, va_list ap);
#endif
#ifndef HAVE_CLOCK_GETTIME
int clock_gettime(int dummy, struct timespec *tv);
#endif
#ifndef HAVE_NANOSLEEP
int nanosleep(const struct timespec *req, struct timespec *rem);
#endif
#ifndef HAVE_PIPE
int pipe(int pipefd[2]);
#endif

#endif
