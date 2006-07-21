#include "config.h"

#ifdef WIN32
# include "win32/stream.c"
#else
# include "unix/stream.c"
#endif
