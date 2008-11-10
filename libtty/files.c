#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "export.h"
#include "utils.h"
#include "formats.h"
#include "libstream/compress.h"

/************************/
/* writing ttyrec files */
/************************/

typedef struct
{
    recorder_info *format;
    void *state;
    FILE *f;
} recorder;


static int find_w_format(char *format, char *filename)
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
            if (rec[nf].ext && match_suffix(filename, rec[nf].ext, skip))
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
    int nf=find_w_format(format, filename);
    
    return (nf==-1) ? 0 : rec[nf].name;
}


export recorder* ttyrec_w_open(int fd, char *format, char *filename, struct timeval *tm)
{
    int nf;
    recorder *r;
    
    nf=find_w_format(format, filename);
    if (nf==-1)
    {
        close(fd);
        return 0;
    }
    
    if (fd==-1)
        fd=open_stream(fd, filename, O_WRONLY|O_CREAT);
    if (fd==-1)
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

/************************/
/* reading ttyrec files */
/************************/

static int find_r_format(char *format, char *filename)
{
    int nf;
    
    if (format)
        for(nf=0; play[nf].name; nf++)
        {
            if (!strcasecmp(format, play[nf].name))
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
        for(nf=0; play[nf].name; nf++)
        {
            if (rec[nf].ext && match_suffix(filename, play[nf].ext, skip))
                break;
        }
    }
    else
        nf=0;
    if (!play[nf].name)
        return -1;
    return nf;
}


export char* ttyrec_r_find_format(char *format, char *filename)
{
    int nf=find_r_format(format, filename);
    
    return (nf==-1) ? 0 : play[nf].name;
}


export char* ttyrec_r_get_format_name(int i)
{
    int j;
    
    if (i<0)
        return 0;
    for(j=0; j<i && play[j].name; j++)
        ;
    return play[j].name;
}


export char* ttyrec_r_get_format_ext(int i)
{
    int j;
    
    if (i<0)
        return 0;
    for(j=0; j<i && play[j].name; j++)
        ;
    return play[j].ext;
}


static void dummyfunc() {}

export int ttyrec_r_play(int fd, char *format, char *filename, 
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg)
{
    int nf;
    
    nf=find_r_format(format, filename);
    if (nf==-1)
    {
        if (fd!=-1)
            close(fd);
        return 0;
    }
    
    if (fd==-1)
        fd=open_stream(fd, filename, O_RDONLY);
    if (fd==-1)
        return 0;
    
    if (!synch_init_wait)
        synch_init_wait=(void*)dummyfunc;
    if (!synch_wait)
        synch_wait=(void*)dummyfunc;
    if (!synch_print)
        synch_print=(void*)dummyfunc;
    
    (*play[nf].play)(fdopen(fd, "rb"), synch_init_wait, synch_wait, synch_print, arg);
    close(fd);
    return 1;
}
