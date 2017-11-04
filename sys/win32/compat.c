#include "config.h"
#include "compat.h"
#include <windows.h>
#include <io.h>
#include <sys/time.h>

#ifndef HAVE_CLOCK_GETTIME
int clock_gettime(int dummy, struct timespec *tv)
{
    ULARGE_INTEGER t;

    GetSystemTimeAsFileTime((LPFILETIME)(void*)&t);
    t.QuadPart -= 116444736000000000;  // 1601-01-01 to Unix epoch
    tv->tv_sec = t.QuadPart / 10000000;
    tv->tv_nsec = t.QuadPart % 10000000 * 100;
    return 0;
}
#endif

#ifndef HAVE_NANOSLEEP
int nanosleep(const struct timespec *req, struct timespec *rem)
{
    // grossly unprecise, but it's not like a human can notice the difference
    int msec = req->tv_sec*1000 + req->tv_nsec/1000000;
    if (msec<=0)
        return 0;
    Sleep(msec);
    return 0;
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
