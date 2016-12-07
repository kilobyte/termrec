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
#include "export.h"
#include "stream.h"
#include "compat.h"
#include "gettext.h"
VISIBILITY_ENABLE
#include "ttyrec.h"
VISIBILITY_DISABLE

#include <sys/socket.h>
#include <sys/un.h>

int connect_unix(const char *socket_path, const char **error)
{
    struct sockaddr_un addr;
    int fd;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        fd = -1;
        *error=strerror(errno);
    }
    return fd;
}


int open_unix(const char* socket_path, int mode, const char **error)
{
    int fd;

    if ((fd=connect_unix(socket_path, error))==-1)
        return -1;

    // no bidi streams
    if (mode&SM_WRITE)
        shutdown(fd, SHUT_RD);
    else
        shutdown(fd, SHUT_WR);

    return fd;
}
