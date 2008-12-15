#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <string.h>
#include <errno.h>
#include "error.h"
#include "export.h"
#include "stream.h"
#include "compat.h"
#include "gettext.h"

/* The "host" arg will be modified! */
/* "port" can be overridden with : */
/* Returns: error message, 0 on success. */
static const char *resolve_host(char *host, int port, struct addrinfo **ai)
{
    long i;
    char *cp;
    int err;
    struct addrinfo hints;
    char portstr[10];
    
    /* free the hostname from IPv6-style trappings: [::1] -> ::1 */
    if (*host=='[')
    {
        host++;
        cp=strchr(host, ']');
        if (!cp)
            return _("Unmatched [ in the host part.");
        *cp++=0;
        if (*cp)
        {
            if (*cp==':')
                goto getrport;
            else
                return _("Cruft after the [host name]."); /* IPv6-style host name */
        }
    }
    /* extract :port */
    if ((cp=strrchr(host, ':')))
    {
    getrport:
        *cp=0;
        cp++; 
        i=strtoul(cp, &cp, 10);
        if (*cp || !i || i>65535)
            return _("Invalid port number");
        port=i;
    }
    else if (port<=0)
        return _("No port number given");
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;
    hints.ai_flags=AI_ADDRCONFIG|AI_NUMERICSERV;
    sprintf(portstr, "%u", port);
    
    if ((err=getaddrinfo(host, portstr, &hints, ai)))
    {
        if (err==EAI_NONAME)
            return _("No such host");
        else
            return gai_strerror(err);
    }
    return 0;
}


static int connect_out(struct addrinfo *ai)
{
    struct addrinfo *addr;
    int sock;
    
    for (addr=ai; addr; addr=addr->ai_next)
    {
        if ((sock=socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol))==-1)
            continue;
    
    intr:
        if ((connect(sock, addr->ai_addr, addr->ai_addrlen)))
        {
            switch(errno)
            {
            case EINTR:
                goto intr;
            default:
                continue;
            }
        }
        return sock;
    }
    return -1;	/* errno will be valid here */
}


#if IS_WIN32
/* workaround socket!=file brain damage */
static void sock2file(int sock, int file)
{
    char buf[BUFSIZ];
    int len;
    
    while((len=recv(sock, buf, BUFSIZ, 0))>0)
        if (write(file, buf, len)!=len)
            return;
    closesocket(sock);
}
#endif


int open_tcp(char* url, int mode, const char **error)
{
    char *cp;
    struct addrinfo *ai;
    int fd;
    
    if (match_prefix(url, "//"))
        url+=2;
    if ((cp=strchr(url, '/')))
        *cp++=0;
    else
        cp="";
    if ((*error=resolve_host(url, 0, &ai)))
        return -1;
    if ((fd=connect_out(ai))==-1)
        *error=strerror(errno);
    freeaddrinfo(ai);
    if (fd==-1)
        return -1;
    
    /* no bidi streams */
    if (mode&M_WRITE)
        shutdown(fd, SHUT_RD);
    else
        shutdown(fd, SHUT_WR);
    
#ifdef IS_WIN32
    return filter(sock2file, fd, !!(mode&M_WRITE));
#else
    return fd;
#endif
}
