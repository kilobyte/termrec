#include <stdio.h>
#include <ttyrec.h>
#include <errno.h>
#include <string.h>
#include "sys/error.h"
#include "gettext.h"

struct timeval tt;

void delay(struct timeval *tm, void *arg)
{
    tadd(tt, *tm);
}

int main(int argc, char **argv)
{
    int i;
    int any=0;
    
    if (argc<2)
        die("%s termtime <%s> ...\n", _("Usage:"), _("filename"));
    for(i=1;i<argc;i++)
    {
        tt.tv_sec=tt.tv_usec=0;
        if (ttyrec_r_play(-1, 0, argv[i], 0, delay, 0, 0))
        {
            printf("%7ld.%06lu\t%s\n", tt.tv_sec, tt.tv_usec, argv[i]);
            any=1;
        }
        else
            fprintf(stderr, "%s: %s\n", argv[i], strerror(errno));
    }
    return !any;
}
