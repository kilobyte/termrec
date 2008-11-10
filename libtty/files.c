#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include "export.h"
#include "utils.h"
#include "formats.h"
#include "libstream/compress.h"

typedef struct
{
    recorder_info *format;
    void *state;
    FILE *f;
} recorder;


static int find_format(char *format, char *filename)
{
    int nf;
    
    if (format)
        for(nf=0; rec[nf].name; nf++)
        {
            if (!strcasecmp(format, rec[nf].name))
                break;
        }
    else if (*filename)
    {
        compress_info *ci;
        int skip;
        
        ci=comp_from_ext(filename, decompressors);
        if (ci)
            skip=strlen(ci->ext);
        else
            skip=0;
        for(nf=0; rec[nf].name; nf++)
        {
            if (match_suffix(filename, rec[nf].ext, skip))
                break;
        }
    }
    else
        nf=0;
    if (!rec[nf].name)
        return -1;
    return nf;
}


export char* ttyrec_w_find_format(char *format, char *filename)
{
    int nf=find_format(format, filename);
    
    return (nf==-1) ? 0 : rec[nf].name;
}


export recorder* ttyrec_w_open(int fd, char *format, char *filename, struct timeval *tm)
{
    int nf;
    recorder *r;
    
    assert(fd!=-1);
    nf=find_format(format, filename);
    if (nf==-1)
        return 0;
    
    r=malloc(sizeof(recorder));
    if (!r)
        return 0;
    
    r->format=&rec[nf];
    r->f=fdopen(fd, "wb");
    assert(r->f);
    r->state=(r->format)->init(r->f, tm);
    return r;
}


export int ttyrec_w_write(recorder *r, struct timeval *tm, char *buf, int len)
{
    assert(r);
    assert(r->f);
    r->format->write(r->f, r->state, tm, buf, len);
    return 0;	/* FIXME */
}


export int ttyrec_w_close(recorder *r)
{
    int err=0;
    
    assert(r);
    assert(r->f);
    r->format->finish(r->f, r->state);
    if (fclose(r->f))
        err=errno;
    free(r);
    return err;
}


export char* ttyrec_w_get_format_name(int i)
{
    int j;
    
    if (i<0)
        return 0;
    for(j=0; j<i && rec[j].name; j++)
        ;
    return rec[j].name;
}


export char* ttyrec_w_get_format_ext(int i)
{
    int j;
    
    if (i<0)
        return 0;
    for(j=0; j<i && rec[j].name; j++)
        ;
    return rec[j].ext;
}
