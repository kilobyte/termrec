#include "config.h"
#include <stdio.h>
#include <unistd.h>
#ifdef IS_WIN32
# include <windows.h>
#endif
#include "utils.h"
#include "stream.h"

typedef void(read_decompressor)(FILE *,int);

void read_bz2(FILE *f, int fd);
void read_gz(FILE *f, int fd);
void write_bz2(FILE *f, int fd);
void write_gz(FILE *f, int fd);

#ifdef IS_WIN32
// A fork(), a fork(), my kingdom for a fork()!
typedef struct
{
    read_decompressor *decomp;
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

void reap_threads()
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
#endif


FILE* stream_open(char *name, char *mode)
{
    FILE *f;
    int len;
    read_decompressor *decomp;
    int p[2];
#ifdef IS_WIN32
    SP args;
    DWORD dummy;
    HANDLE th;
    HANDLE sem;
#endif
    int wr=*mode!='r';
    
    f=fopen(name, mode);
    if (!f)
        return 0;
    
    len=strlen(name);
#if (defined HAVE_LIBBZ2) || (defined SHIPPED_LIBBZ2)
    if (len>4 &&
          name[len-4]=='.' &&
          name[len-3]=='b' &&
          name[len-2]=='z' &&
          name[len-1]=='2')
        decomp=wr?write_bz2:read_bz2;
    else
#endif
#if (defined HAVE_LIBZ) || (defined SHIPPED_LIBZ)
    if (len>3 &&
          name[len-3]=='.' &&
          name[len-2]=='g' &&
          name[len-1]=='z')
        decomp=wr?write_gz:read_gz;
    else
#endif
    return f;

#ifdef IS_WIN32
# ifdef __CYGWIN__
#warning stream_in: using CygWin path
    if (pipe(p))
    {
        fclose(f);
        return 0;
    }
# else
#warning stream_in: using Win32-native path
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
    if (wr)
        register_thread(th);
    else
        CloseHandle(th);
    WaitForSingleObject(sem, INFINITE);
    CloseHandle(sem);
#else
#warning stream_in: using Unix path
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
#endif
    return fdopen(p[wr], mode);
}
