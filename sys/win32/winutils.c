#include <windows.h>
#include <stdio.h>
#include "error.h"
#include "winutils.h"

#define BUFFER_SIZE 1024

void die(const char *txt, ...)
{
    va_list ap;
    char buf[BUFFER_SIZE];
    const char *errstr;
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
