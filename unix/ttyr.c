#include "config.h"
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include "pty.h"
#include "utils.h"
#include "name_out.h"
#include "formats.h"

volatile int need_resize;
struct termios ta;
int ptym;
FILE *record_f;
void* record_state;

void sigwinch(int s)
{
    need_resize=1;
}

void sigpass(int s)
{
    kill(ptym, s);
}

void setsignals()
{
    struct sigaction act;
    
    sigemptyset(&act.sa_mask);
    act.sa_flags=SA_RESTART;
    act.sa_handler=sigwinch;
    if (sigaction(SIGWINCH,&act,0))
        error("sigaction SIGWINCH");
    act.sa_handler=sigpass;
    if (sigaction(SIGINT,&act,0))
        error("sigaction SIGINT");
    if (sigaction(SIGHUP,&act,0))
        error("sigaction SIGHUP");
    if (sigaction(SIGQUIT,&act,0))
        error("sigaction SIGQUIT");
    if (sigaction(SIGTERM,&act,0))
        error("sigaction SIGTERM");
    if (sigaction(SIGTSTP,&act,0))
        error("sigaction SIGTSTP");
}

void resize()
{
    struct winsize ts;
    struct timeval tv;
    char buf[20];

    if (raw)
        return;
    if (ioctl(1,TIOCGWINSZ,&ts))
        return;
    if (!ts.ws_row || !ts.ws_col)
        return;
    ioctl(ptym,TIOCSWINSZ,&ts);
    gettimeofday(&tv, 0);
    (*rec[codec].write)(record_f, record_state, &tv, buf, snprintf(buf, sizeof(buf),
        "\e[8;%d;%dt", ts.ws_row, ts.ws_col));
}

void tty_raw()
{
    struct termios rta;

    memset(&ta, 0, sizeof(ta));
    tcgetattr(0, &ta);
    
    rta=ta;
    pty_makeraw(&rta);
    tcsetattr(0, TCSADRAIN, &rta);
}

void tty_restore()
{
    tcsetattr(0, TCSADRAIN, &ta);
}

#define BS 65536

int main(int argc, char **argv)
{
    fd_set rfds;
    char buf[BS];
    int r;
    struct timeval tv;

    get_parms(argc, argv, 0);
    record_f=fopen_out(&record_name, 1);
    if (!record_f)
    {
        fprintf(stderr, "Can't open %s\n", record_name);
        return 1;
    }
    if (!command)
        command=getenv("SHELL");
    if (!command)
        command="/bin/sh";

    if ((ptym=run(command,0,0))==-1)
    {
        fprintf(stderr, "Couldn't allocaty pty.\n");
        return 1;
    }
    
    gettimeofday(&tv, 0);
    record_state=(*rec[codec].init)(record_f, &tv);
    need_resize=1;
 
    setsignals();
    tty_raw();
    
    if (!raw && is_utf8())
        (*rec[codec].write)(record_f, record_state, &tv, "\e%G", 3);
    
    while(1)
    {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(ptym, &rfds);
        r=select(ptym+1, &rfds, 0, 0, 0);
        
        if (need_resize)
        {
            need_resize=0;
            resize();
        }
        
        switch(r)
        {
        case 0:
            continue;
        case -1:
            if (errno==EINTR)
                continue;
            tty_restore();
            error("select()");
        }
        
        if (FD_ISSET(0, &rfds))
        {
            r=read(0, buf, BS);
            if (r<=0)
            {
                close(ptym);
                break;
            }
            write(ptym, buf, r);
        }
        if (FD_ISSET(ptym, &rfds))
        {
            r=read(ptym, buf, BS);
            if (r<=0)
            {
                close(ptym);
                break;
            }
            gettimeofday(&tv, 0);
            (*rec[codec].write)(record_f, record_state, &tv, buf, r);
            write(1, buf, r);
        }
    }

    (*rec[codec].finish)(record_f, record_state);
    fclose(record_f);
    tty_restore();
    wait(0);
    return 0;
}
