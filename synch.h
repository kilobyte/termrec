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

void synch_init_wait(struct timeval *ts);
void synch_wait(struct timeval *tv);
void synch_print(char *buf, int len);
