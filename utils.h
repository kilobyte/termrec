#include "config.h"
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef WIN32
# include <windows.h>
#endif

#if !HAVE_DECL_O_BINARY
# define O_BINARY 0
#endif

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

#ifndef HAVE_ASPRINTF
int asprintf(char **sptr, const char *fmt, ...);
#endif

#ifdef IS_WIN32
# ifndef HAVE_GETTIMEOFDAY
void gettimeofday(struct timeval *tv, void * dummy);
# endif
# ifndef HAVE_USLEEP
void usleep(unsigned int usec);
# endif
#endif
void show_error(char *blah);

void tadd(struct timeval *t, struct timeval *d);
void tsub(struct timeval *t, struct timeval *d);
void tmul(struct timeval *t, int m);
void tdiv(struct timeval *t, int m);
#define tcmp(t1, t2)	(((t1).tv_sec>(t2).tv_sec)?1:		\
                         ((t1).tv_sec<(t2).tv_sec)?-1:		\
                         ((t1).tv_usec>(t2).tv_usec)?1:		\
                         ((t1).tv_usec<(t2).tv_usec)?-1:0)

int match_suffix(char *txt, char *ext, int skip);
void error(const char *txt, ...);

#ifdef WIN32
int message_loop(HANDLE* lphObjects, int cObjects);
# undef stderr
# define stderr stdout
#endif
