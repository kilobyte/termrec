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

int match_suffix(char *txt, char *ext, int skip);
int match_prefix(char *txt, char *ext);

#ifdef WIN32
# include "sys/win32/winutils.h"
#else
int is_utf8();
#endif
