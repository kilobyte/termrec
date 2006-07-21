#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

void sockets_init();

#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND

int getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res);

void freeaddrinfo(struct addrinfo *res);

const char *gai_strerror(int errcode);
