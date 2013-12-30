#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include "config.h"

#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND

#ifndef HAVE_WIN32_GETADDRINFO
int getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
#endif

#ifndef HAVE_WIN32_GAI_STRERROR
const char *gai_strerror(int errcode);
#endif
