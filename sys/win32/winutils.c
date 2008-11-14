#include <windows.h>
#include <io.h>
#include <stdio.h>

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
    Sleep(usec/1000);
}
# endif

#ifndef HAVE_PIPE
int pipe(int p[2])
{
    if (!CreatePipe((PHANDLE)p, (PHANDLE)p+1, 0, 0))
        return 0;
    p[0]=_open_osfhandle(p[0],0);
    p[1]=_open_osfhandle(p[1],0);
    return 1;
}
#endif

#define BUFFER_SIZE 1024

void die(const char *txt, ...)
{
    va_list ap;
    char buf[BUFFER_SIZE], *errstr;
    int len;
    DWORD err=GetLastError();

    va_start(ap, txt);
    len=vsnprintf(buf, BUFFER_SIZE, txt, ap);
    va_end(ap);
    
    if (len && buf[len-1]!='\n')
    {
        FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        err,
        0,
        (LPTSTR) &errstr,
        0, NULL);
        if (!err)
            errstr="unknown error";
        snprintf(buf+len, BUFFER_SIZE-len, ": %s\n", errstr);
    }
    
    MessageBox(0, buf, "TermRec", MB_ICONERROR);
    exit(1);
}


// Returns -1 for WM_QUIT or the index into the Objects array.
int message_loop(HANDLE* lphObjects, int cObjects)
{
    while (TRUE)
    {
        DWORD result;
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                return -1;
            DispatchMessage(&msg);
        }

        // Wait for any message sent or posted to this queue
        // or for one of the passed handles be set to signaled.
        result = MsgWaitForMultipleObjects(cObjects, lphObjects,
                 FALSE, INFINITE, QS_ALLINPUT);

        if (result == (WAIT_OBJECT_0 + cObjects))
        {
            // New messages have arrived.
            continue;
        }
        else
        {
            // One of the handles became signaled.
            return result-WAIT_OBJECT_0;
        }
    }
}
