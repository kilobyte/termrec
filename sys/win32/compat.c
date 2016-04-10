#include "config.h"
#include <windows.h>
#include <io.h>
#include <sys/time.h>

#ifndef HAVE_GETTIMEOFDAY
void gettimeofday(struct timeval *tv, void * dummy)
{
    ULARGE_INTEGER t;

    GetSystemTimeAsFileTime((LPFILETIME)(void*)&t);
    t.QuadPart/=10;
    tv->tv_sec=t.QuadPart/1000000;
    tv->tv_usec=t.QuadPart%1000000;
}
#endif

#ifndef HAVE_USLEEP
void usleep(unsigned int usec)
{
    if (usec<0)
        return;
    Sleep(usec/1000);
}
#endif

#ifndef HAVE_PIPE
int pipe(int p[2])
{
    if (!CreatePipe((PHANDLE)p, (PHANDLE)p+1, 0, 0))
        return -1;
    p[0]=_open_osfhandle(p[0],0);
    p[1]=_open_osfhandle(p[1],0);
    return 0;
}
#endif

#ifndef HAVE_USELECT
void uselect(struct timeval *timeout)
{
    if (!timeout || timeout->tv_sec<0)
        return;
    Sleep(timeout->tv_sec*1000+timeout->tv_usec/1000);
}
#endif
