#include <string.h>
#include <unistd.h>
#include "config.h"
#include "error.h"
#include "threads.h"
#include "export.h"
#include "compress.h"
#include "stream.h"
#include "compat.h"


// disabled: #ifdef HAVE_FORK
#if 0
int filter(void func(int,int,char*), int fd, int wr, char *arg, char **error)
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
        *error="fork() failed";
        close(fd);
        close(p[0]);
        close(p[1]);
        return -1;
    case 0:
        close(p[wr]);
        func(fd, p[!wr], arg);
        exit(0);
    default:
        close(p[!wr]);
    }
    return p[wr];
}
#else
struct filterdata
{
    int fdin, fdout;
    void (*func)(int,int,char*);
    char *arg;
};


static int nstreams=-1;
static cond_t exitcond;
static mutex_t nsm;

static void filterthr(struct filterdata *args)
{
    int fdin, fdout;
    void (*func)(int,int,char*);
    char *arg;
    
    fdin=args->fdin;
    fdout=args->fdout;
    func=args->func;
    arg=args->arg;
    free(args);
    
    func(fdin, fdout, arg);
    mutex_lock(nsm);
    if (!--nstreams)
        cond_wake(exitcond);
    mutex_unlock(nsm);
}


static void finish_up()
{
    mutex_lock(nsm);
    while (nstreams)
    {
        cond_wait(exitcond, nsm);
    }
    mutex_unlock(nsm);
}


int filter(void func(int,int,char*), int fd, int wr, char *arg, char **error)
{
    int p[2];
    struct filterdata *fdata;
    thread_t th;
    
    if (pipe(p))
        goto failpipe;
    if (!(fdata=malloc(sizeof(struct filterdata))))
        goto failfdata;
    fdata->fdin=fd;
    fdata->fdout=p[!wr];
    fdata->func=func;
    fdata->arg=arg;
    
    if (nstreams==-1)
    {
        mutex_init(nsm);
        cond_init(exitcond);
        nstreams=0;
        atexit(finish_up);
    }
    
    mutex_lock(nsm);
    if (thread_create_detached(&th, filterthr, fdata))
        goto failthread;
    nstreams++;
    mutex_unlock(nsm);
    return p[wr];
failthread:
    *error="Couldn't create thread";
    mutex_unlock(nsm);
    free(fdata);
failfdata:
    close(p[0]);
    close(p[1]);
failpipe:
    close(fd);
    return -1;
}
#endif


export int open_stream(int fd, char* url, int mode, char **error)
{
    int wr= !!(mode&M_WRITE);
    compress_info *ci;
    char *dummy;
    
    if (fd==-1)
    {
        if (!url)
            return -1;
        if (!error)
            error=&dummy;
        if (!strcmp(url, "-"))
            fd=dup(wr? 1 : 0);
        else if (match_prefix(url, "file://"))
            fd=open_file(url+7, mode, error);
        else if (match_prefix(url, "tcp://"))
            fd=open_tcp(url+6, mode, error);
        else if (match_prefix(url, "telnet://"))
            fd=open_telnet(url+9, mode, error);
        else if (match_prefix(url, "termcast://"))
            fd=open_termcast(url+11, mode, error);
        else
            fd=open_file(url, mode, error);
    }
    if (fd==-1)
        return -1;
    
    ci=comp_from_ext(url, wr? compressors : decompressors);
    if (!ci)
        return fd;
    return filter(ci->comp, fd, wr, 0, error);
}
