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

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

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
# include "sys/win32/winutils.h"
#else
int is_utf8();
#endif
