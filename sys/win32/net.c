#include "net.h"

#ifndef HAVE_WIN32_GETADDRINFO
static void* LoadFunc(char *dll, char *name)
{
    HMODULE hdll;

    hdll=LoadLibrary(dll);
    if (!hdll)
        return 0;

    return GetProcAddress(hdll, name);
}

typedef int (WINAPI *PGETADDRINFO) (
    IN  const char                      *nodename,
    IN  const char                      *servname,
    IN  const struct addrinfo           *hints,
    OUT struct addrinfo                 **res);

static PGETADDRINFO pgetaddrinfo;
static int initialized=0;

static void sockets_init()
{
    WSADATA wsaData;

    WSAStartup(MAKEWORD(2,2), &wsaData);

    if (!(pgetaddrinfo=LoadFunc("ws2_32", "getaddrinfo"))) // WinXP+
        pgetaddrinfo=LoadFunc("wship6", "getaddrinfo"); // Win2K
}


// Try the real stuff first; if not available, emulate it with BSD IPv4
int getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res)
{
    struct hostent *hp;
    struct sockaddr_in *sin;

    if (!initialized)
        sockets_init();

    if (pgetaddrinfo)
        return (*pgetaddrinfo)(node, service, hints, res);

    if (!(hp=gethostbyname(node)))
    {
        switch (h_errno)
        {
        case HOST_NOT_FOUND:
            return EAI_NONAME;
        case NO_DATA:
            return EAI_NODATA;
        case NO_RECOVERY:
            return EAI_FAIL;
        case TRY_AGAIN:
            return EAI_AGAIN;
        }
        return -1; // ?
    }

    *res=malloc(sizeof(struct addrinfo));
    memset(*res, 0, sizeof(struct addrinfo));
    (*res)->ai_family=AF_INET;
    (*res)->ai_socktype=SOCK_STREAM;
    (*res)->ai_protocol=IPPROTO_TCP;
    (*res)->ai_addrlen=sizeof(struct sockaddr_in);
    sin=malloc(sizeof(struct sockaddr_in));
    (*res)->ai_addr=(struct sockaddr *)sin;
    sin->sin_family=AF_INET;
    memcpy((char *)&sin->sin_addr, hp->h_addr, sizeof(sin->sin_addr));
    sin->sin_port=htons(atoi(service));

    return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
    // not used, we need the info till the end
}
#endif

#ifndef HAVE_WIN32_GAI_STRERROR
const char *gai_strerror(int err)
{
    DWORD dwMsgLen;
    static char buff[1024 + 1];

    dwMsgLen = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM
                             |FORMAT_MESSAGE_IGNORE_INSERTS
                             |FORMAT_MESSAGE_MAX_WIDTH_MASK,
                              NULL,
                              err,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPSTR)buff,
                              1024,
                              NULL);

    return buff;
}
#endif
