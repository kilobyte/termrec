#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "config.h"
#include "error.h"
#include "export.h"
#include "stream.h"
#include "compat.h"


int open_file(char* url, int mode)
{
    int fmode, fd;

    switch (mode)
    {
    case M_READ:
        fmode=O_RDONLY;
        break;
    case M_WRITE:
        fmode=O_WRONLY|O_CREAT|O_TRUNC;
        break;
    case M_REPREAD:
        fmode=O_RDONLY;
        break;
    case M_APPEND:
        fmode=O_WRONLY|O_APPEND|O_CREAT;
        break;
    default:
        return -1;
    }
    fd=open(url, fmode|O_BINARY, 0666);

    return fd;
}
