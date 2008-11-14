#ifndef _COMPAT_H
#define _COMPAT_H

#include "config.h"

#ifdef WIN32
# include "win32/net.h"
#else
# define closesocket(x) close(x)
#endif

#ifndef HAVE_ASPRINTF
int asprintf(char **sptr, const char *fmt, ...);
int vasprintf(char **sptr, const char *fmt, va_list ap);
#endif
#ifndef HAVE_GETTIMEOFDAY
void gettimeofday(struct timeval *tv, void * dummy);
#endif
#ifndef HAVE_USLEEP
void usleep(unsigned int usec);
#endif
#ifndef HAVE_PIPE
int pipe(int pipefd[2]);
#endif

#endif
