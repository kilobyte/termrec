#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "error.h"
#include "export.h"
#include "stream.h"
#include "compat.h"


int open_file(const char* url, int mode, const char **error)
{
    int fmode, fd;

    switch (mode)
    {
    case SM_READ:
        fmode=O_RDONLY;
        break;
    case SM_WRITE:
        fmode=O_WRONLY|O_CREAT|O_TRUNC;
        break;
    case SM_REPREAD:
        fmode=O_RDONLY;
        break;
    case SM_APPEND:
        fmode=O_WRONLY|O_APPEND|O_CREAT;
        break;
    default:
        *error="unknown file mode in open_stream(file://)";
        return -1;
    }
    if ((fd=open(url, fmode|O_BINARY, 0666))==-1)
        *error=strerror(errno);

    return fd;
}
