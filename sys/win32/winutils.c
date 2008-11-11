#include <windows.h>
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

void show_error(char *blah) 
{ 
    TCHAR szBuf[80]; 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    wsprintf(szBuf, 
        "%s failed with error %d: %s", 
        blah, dw, lpMsgBuf); 
 
    printf(szBuf);

    LocalFree(lpMsgBuf);
}

// Returns -1 for WM_QUIT or the index into the Objects array.
int message_loop(HANDLE* lphObjects, int cObjects)
{ 
    while (TRUE)
    {
        DWORD result ; 
        MSG msg ; 

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

void error(const char *txt, ...)
{
    va_list ap;
    char buf[1024];

    va_start(ap, txt);
    vsnprintf(buf, 1024, txt, ap);
    MessageBox(0, buf, "TermRec", MB_ICONERROR);
    exit(1);
    va_end(ap); /* make ugly compilers happy */
}
