#ifndef _NET_H
#define _NET_H

#include "config.h"

#ifdef WIN32
# include "win32/net.h"
#else
# define sockets_init()
# define closesocket(x) close(x)
#endif

#endif
