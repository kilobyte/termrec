#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "utils.h"
#include "stream.h"


FILE* stream_open(FILE *f, char *name, char *mode, compress_info *comptable, int nodetach)
{
    compress_func *decomp;
    int p[2];
    compress_info *ci;
    int wr=*mode!='r';
    
    if (!f)
        return 0;

    decomp=0;
    for(ci=comptable;ci->name;ci++)
        if (match_suffix(name, ci->ext, 0))
        {
            decomp=ci->comp;
            break;
        }
    if (!decomp)
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
        (*decomp)(f, p[!wr]);
        exit(0);
    default:
        close(p[!wr]);
    }
    return fdopen(p[wr], mode);
}

void stream_reap_threads()
{
}
