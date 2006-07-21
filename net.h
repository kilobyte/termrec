#ifndef _NET_H
#define _NET_H

#include "config.h"

#ifdef WIN32
# include "win32/net.h"
/* we detect this on runtime */
# define IPV6
#else
# define sockets_init()
# define closesocket(x) close(x)
#endif

#ifdef HAVE_GETADDRINFO
# define IPV6
#endif

#endif
