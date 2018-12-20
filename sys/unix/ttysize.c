#include "config.h"
#include <unistd.h>
#if HAVE_TERMIOS_H
# include <termios.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include "export.h"
#include "ttysize.h"

int get_tty_size(int fd, int *x, int *y)
{
    struct winsize ts;

    if (!isatty(fd))
        return 0;
    if (!ioctl(fd,TIOCGWINSZ,&ts) && ts.ws_row>0 && ts.ws_col>0)
    {
        *x=ts.ws_col;
        *y=ts.ws_row;
        return 1;
    }
    return 0;
}
