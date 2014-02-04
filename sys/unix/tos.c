#include "config.h"
#if HAVE_TERMIOS_H
# include <termios.h>
# include <unistd.h>
#endif

#include "sys/unix/uutils.h"

static struct termios old_tattr;

void kbd_raw()
{
    struct termios tattr;

    if (!isatty(0))
        return;

    tcgetattr(0,&old_tattr);
    tattr=old_tattr;
    // cfmakeraw(&tattr);
    tattr.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                       |INLCR|IGNCR|ICRNL|IXON);
    tattr.c_oflag &= ~OPOST;
    tattr.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    tattr.c_cflag &= ~(CSIZE|PARENB);
    tattr.c_cflag |= CS8;

#ifndef IGNORE_INT
    tattr.c_lflag|=ISIG;        // allow C-c, C-\ and C-z
#endif
    tattr.c_cc[VMIN]=1;
    tattr.c_cc[VTIME]=0;
    tcsetattr(0,TCSANOW,&tattr);
}

void kbd_restore()
{
    tcdrain(0);
    tcsetattr(0,TCSADRAIN,&old_tattr);
}
