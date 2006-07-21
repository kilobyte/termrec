#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <windows.h>
#include "utils.h"
#include "stream.h"

// A fork(), a fork(), my kingdom for a fork()!
typedef struct
{
    compress_func *decomp;
    FILE *f;
    int fd;
    HANDLE sem;
} SP, *LPSP;

WINAPI void StreamThreadFunc(LPSP arg)
{
    SP args=*arg;
    
    ReleaseSemaphore(args.sem, 1, 0);    
    (*(args.decomp))(args.f, args.fd);
    close(args.fd);
}


HANDLE *threads=0;
int nthreads=0;

void register_thread(HANDLE th)
{
    threads=realloc(threads, (nthreads+1)*sizeof(HANDLE));
    threads[nthreads++]=th;
}

void stream_reap_threads()
{
    if (threads)
    {
        if (WaitForMultipleObjects(nthreads, threads, 1, INFINITE)==WAIT_FAILED)
            show_error("WaitForMultipleObjects");
        free(threads);
        threads=0;
        nthreads=0;
    }
}


FILE* stream_open(FILE *f, char *name, char *mode, compress_info *comptable, int nodetach)
{
    compress_func *decomp;
    int p[2];
    compress_info *ci;
    SP args;
    DWORD dummy;
    HANDLE th;
    HANDLE sem;
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

#ifdef __CYGWIN__
#warning stream_open: using CygWin path
    if (pipe(p))
    {
        fclose(f);
        return 0;
    }
#else
#warning stream_open: using Win32-native path
    if (!CreatePipe((PHANDLE)p, (PHANDLE)p+1, 0, 0))
    {
        fclose(f);
        return 0;
    }
    p[0]=_open_osfhandle(p[0],0);
    p[1]=_open_osfhandle(p[1],0);
#endif
    args.decomp=decomp;
    args.f=f;
    args.fd=p[!wr];
    sem=CreateSemaphore(0,0,1,0);
    args.sem=sem;
    th=CreateThread(0, 0, (LPTHREAD_START_ROUTINE)StreamThreadFunc, (LPVOID)&args,
        0, &dummy);
    if (nodetach)
        register_thread(th);
    else
        CloseHandle(th);
    WaitForSingleObject(sem, INFINITE);
    CloseHandle(sem);
    return fdopen(p[wr], mode);
}
