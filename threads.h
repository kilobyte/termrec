#include "config.h"

#ifdef WIN32
# include "win32/threads.h"
#else
# include "unix/threads.h"
#endif
