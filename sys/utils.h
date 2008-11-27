#include "config.h"
#include "sys/error.h"

#ifdef WIN32
# include "sys/win32/winutils.h"
#else
# include "sys/unix/uutils.h"
#endif

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
