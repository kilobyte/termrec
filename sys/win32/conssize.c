#include "config.h"
#include <windows.h>
#include <io.h>
#include "export.h"
#include "ttysize.h"

int get_tty_size(int fd, int *x, int *y)
{
    CONSOLE_SCREEN_BUFFER_INFO bi;
    HANDLE h;

    if (!(h=(HANDLE)_get_osfhandle(fd)))
        return 0;
    if (!GetConsoleScreenBufferInfo(h, &bi))
        return 0;
    *x=bi.srWindow.Right-bi.srWindow.Left;
    *y=bi.srWindow.Bottom-bi.srWindow.Top;
    return 1;
}
