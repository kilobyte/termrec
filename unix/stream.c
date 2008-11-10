#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "utils.h"


FILE* stream_open(FILE *f, char *name, char *mode, compress_info *ci, int nodetach)
{
    int p[2];
    int wr=*mode!='r';
    
    if (!f)
        return 0;
    
    ci=comp_from_ext(name, ci);
    if (!ci)
        return f;

    pipe(p);
    switch(fork())
    {
    case -1:
        fclose(f);
        close(p[0]);
        close(p[1]);
        return 0;
    case 0:
        close(p[wr]);
        ci->comp(f, p[!wr]);
        exit(0);
    default:
        close(p[!wr]);
    }
    return fdopen(p[wr], mode);
}

void stream_reap_threads()
{
}
