#include <windows.h>

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
