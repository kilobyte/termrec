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
#include "formats.h"
#include "libstream/compress.h"
#include "libstream/stream.h"

/************************/
/* writing ttyrec files */
/************************/

typedef struct
{
    recorder_info *format;
    void *state;
    FILE *f;
} recorder;


static int find_w_format(const char *format, const char *filename, const char *fallback)
{
    int nf;

    // given explicitely
    if (format)
    {
        for (nf=0; rec[nf].name; nf++)
            if (!strcasecmp(format, rec[nf].name))
                return nf;
        return -1;      // no fallback to default
    }
    // guess from the filename
    else if (filename)
    {
        compress_info *ci;
        int skip;

        ci=comp_from_ext(filename, decompressors);
        if (ci)
            skip=strlen(ci->ext);
        else
            skip=0;
        for (nf=0; rec[nf].name; nf++)
            if (rec[nf].ext && match_suffix(filename, rec[nf].ext, skip))
                return nf;
    }
    // is the default valid?
    for (nf=0; rec[nf].name; nf++)
        if (!strcasecmp(fallback, rec[nf].name))
            return nf;
    return -1;
}


export const char* ttyrec_w_find_format(const char *format, const char *filename, const char *fallback)
{
    int nf=find_w_format(format, filename, fallback);

    return (nf==-1) ? 0 : rec[nf].name;
}


export recorder* ttyrec_w_open(int fd, const char *format, const char *filename, const struct timeval *tm)
{
    int nf;
    recorder *r;
    struct timeval t0={0,0};

    nf=find_w_format(format, filename, "ansi");
    if (nf==-1)
    {
        close(fd);
        return 0;
    }

    if (fd==-1)
        fd=open_stream(fd, filename, SM_WRITE, 0);
    if (fd==-1)
        return 0;

    r=malloc(sizeof(recorder));
    if (!r)
    {
        close(fd);
        return 0;
    }

    r->format=&rec[nf];
    r->f=fdopen(fd, "wb");
    assert(r->f);
    r->state=(r->format)->init(r->f, tm?tm:&t0);
    return r;
}


export int ttyrec_w_write(recorder *r, const struct timeval *tm, const char *buf, int len)
{
    assert(r);
    assert(r->f);
    r->format->write(r->f, r->state, tm, buf, len);
    return !ferror(r->f);
}


export int ttyrec_w_close(recorder *r)
{
    assert(r);
    assert(r->f);
    r->format->finish(r->f, r->state);
    int ok=!ferror(r->f);
    ok&=!fclose(r->f);
    free(r);
    return ok;
}


export const char* ttyrec_w_get_format_name(int i)
{
    if (i<0 || i>=rec_n)
        return 0;
    return rec[i].name;
}


export const char* ttyrec_w_get_format_ext(const char *format)
{
    int i = find_w_format(format, 0, 0);
    if (i<0 || i>=rec_n)
        return 0;
    return rec[i].ext;
}

/************************/
/* reading ttyrec files */
/************************/

static int find_r_format(const char *format, const char *filename, const char *fallback)
{
    int nf;

    // given explicitely
    if (format)
    {
        for (nf=0; play[nf].name; nf++)
            if (!strcasecmp(format, play[nf].name))
                return nf;
        return -1;      // no fallback to default
    }
    // guess from the filename
    else if (filename)
    {
        compress_info *ci;
        int skip;

        ci=comp_from_ext(filename, compressors);
        if (ci)
            skip=strlen(ci->ext);
        else
            skip=0;
        for (nf=0; play[nf].name; nf++)
            if (play[nf].ext && match_suffix(filename, play[nf].ext, skip))
                return nf;
    }
    // is the default valid?
    for (nf=0; play[nf].name; nf++)
        if (!strcasecmp(fallback, play[nf].name))
            return nf;
    return -1;
}


export const char* ttyrec_r_find_format(const char *format, const char *filename, const char *fallback)
{
    int nf=find_r_format(format, filename, fallback);

    return (nf==-1) ? 0 : play[nf].name;
}


export const char* ttyrec_r_get_format_name(int i)
{
    if (i<0 || i>=play_n)
        return 0;
    return play[i].name;
}


export const char* ttyrec_r_get_format_ext(const char *format)
{
    int i = find_r_format(format, 0, 0);
    if (i<0 || i>=play_n)
        return 0;
    return play[i].ext;
}


static void dummyfunc() {}

export int ttyrec_r_play(int fd, const char *format, const char *filename,
    void *(synch_init_wait)(const struct timeval *ts, void *arg),
    void *(synch_wait)(const struct timeval *tv, void *arg),
    void *(synch_print)(const char *buf, int len, void *arg),
    void *arg)
{
    int nf;
    FILE *f;

    nf=find_r_format(format, filename, "auto");
    if (nf==-1)
    {
        if (fd!=-1)
            close(fd);
        return 0;
    }

    if (fd==-1)
        fd=open_stream(fd, filename, SM_READ, 0);
    if (fd==-1)
        return 0;

    if (!synch_init_wait)
        synch_init_wait=(void*)dummyfunc;
    if (!synch_wait)
        synch_wait=(void*)dummyfunc;
    if (!synch_print)
        synch_print=(void*)dummyfunc;

    f=fdopen(fd, "rb");
    assert(f);
    (*play[nf].play)(f, synch_init_wait, synch_wait, synch_print, arg, 0);
    fclose(f);
    return 1;
}
