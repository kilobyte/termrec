#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "compress.h"
#include "stream.h"
#include "export.h"
#include "error.h"


#ifdef HAVE_FORK
int filter(void func(int,int), int fd, int wr)
{
    int p[2];
    
    if (pipe(p))
    {
        close(fd);
        return -1;
    }
    switch(fork())
    {
    case -1:
        close(fd);
        close(p[0]);
        close(p[1]);
        return -1;
    case 0:
        close(p[wr]);
        func(fd, p[!wr]);
        exit(0);
    default:
        close(p[!wr]);
    }
    return p[wr];
}
#else
int filter(void func(int,int), int fd, int wr)
{
    return -1;
}
#endif


export int open_stream(int fd, char* url, int mode)
{
    int wr= (mode&(O_RDONLY|O_WRONLY|O_RDWR))!=O_RDONLY;
    compress_info *ci;
    
    if (fd==-1)
    {
        if (!url)
            return -1;
        if (!strcmp(url, "-"))
            fd=dup(wr? 1 : 0);
        else
            fd=open(url, mode, 0666);
    }
    if (fd==-1)
        return -1;
    
    ci=comp_from_ext(url, wr? compressors : decompressors);
    if (!ci)
        return fd;
    return filter(ci->comp, fd, wr);
}
