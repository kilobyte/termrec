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
#endif

#endif
