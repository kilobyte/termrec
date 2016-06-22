#include <stdio.h>
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#if HAVE_TERMIOS_H
# include <termios.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_GRANTPT
# ifdef HAVE_STROPTS_H
#  include <stropts.h>
# endif
#endif

#ifndef HAVE_FORKPTY
extern int forkpty(int *amaster,char *dummy,struct termios *termp, struct winsize *wp);
#endif

extern void pty_resize(int fd, int sx, int sy);
extern int run(const char *command, int sx, int sy);
extern void pty_makeraw(struct termios *ta);
