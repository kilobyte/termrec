#define _GNU_SOURCE
#include "config.h"
#include <vt100.h>
#include <ttyrec.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include "_stdint.h"
#include "gettext.h"
#include "common.h"
#include "sys/threads.h"
#include "sys/utils.h"
#include "play/player.h"


#if (defined HAVE_GETOPT_LONG) && (defined HAVE_GETOPT_H)
static struct option play_opts[]={
{"format",	1, 0, 'f'},
{"speed",	1, 0, 's'},
{"follow",	0, 0, 'p'},
{"help",	0, 0, 'h'},
{0,		0, 0, 0},
};
#endif

static char *play_name, *format;
static int follow;

static void get_play_parms(int argc, char **argv)
{
    char *ep;

    play_name=0;
    follow=0;
    speed=1000;

    while (1)
    {
#if (defined HAVE_GETOPT_LONG) && (defined HAVE_GETOPT_H)
        switch(getopt_long(argc, argv, "f:s:ph", play_opts, 0))
#else
        switch(getopt(argc, argv, "f:s:ph"))
#endif
        {
        case -1:
            goto finish_args;
        case ':':
        case '?':
            exit(1);
        case 'f':
            get_r_format(&format);
            break;
        case 's':
            speed=strtof(optarg, &ep)*1000;
            if (*ep || ep==optarg || speed<=0 || speed>1000000)
                die(_("Please provide a valid number <=1000 after -s.\n"));
            break;
        case 'p':
            follow=1;
            break;
        case 'h':
            printf(
                "%stermplay [-f format] [file]\n"
                "    %s"
                "-f, --format X        %s\n"
                "-s, --speed X         %s\n"
                "-p, --follow          %s\n"
                "-h, --help            %s\n"
                "%s%s",
                _("Usage: "),
                _("Replays a console session from a file.\n"),
                _("set input format to X (-f whatever for the list)"),
                _("set initial speed to X (default is 1.0)"),
                _("keep trying to read a growing file (like tail -f)"),
                _("show this usage message"),
                _("If no format is given, it will be set according to the extension of the\n"
                  "    filename.\n"),
                _("You don't have to uncompress .gz and .bz2 files first.\n"));
            exit(0);
        }
    }
finish_args:
    if (optind<argc)
        play_name=argv[optind++];
    else
        die("%s termplay <%s>\n", _("Usage:"), _("filename"));
    if (optind<argc)
        die(_("You specified the file to play multiple times.\n"));

    if (!format)
        format=ttyrec_r_find_format(0, play_name, "auto");
}


static void loader_end(void *arg)
{
    mutex_lock(waitm);
    loaded=1;
    if (waiting)
    {
        waiting=0;
        cond_wake(waitc);
    }
    mutex_unlock(waitm);
}

static struct timeval lt;

static void loader_init_wait(struct timeval *ts, void *arg)
{
//    lt=*ts;
}

static void loader_wait(struct timeval *delay, void *arg)
{
    tadd(lt, *delay);
}

static void loader_print(char *data, int len, void *arg)
{
    ttyrec_add_frame(tr, &lt, data, len);
    lt.tv_sec=lt.tv_usec=0;
    mutex_lock(waitm);
    if (waiting)
    {
        waiting=0;
        cond_wake(waitc);
    }
    mutex_unlock(waitm);
}

static int loader(void *arg)
{
    pthread_cleanup_push(loader_end, 0);
    ttyrec_r_play((intptr_t)arg, format, play_name,   loader_init_wait, loader_wait, loader_print, 0);
    pthread_cleanup_pop(1);
    return 1;
}


int main(int argc, char **argv)
{
    int fd;
    thread_t loadth;
    char *error;

    get_play_parms(argc, argv);
    if ((fd=open_stream(-1, play_name, follow?SM_REPREAD:SM_READ, &error))==-1)
        die("%s: %s\n", play_name, error);
    tr=ttyrec_init(vt100_init(200, 100, 1));
    mutex_init(waitm);
    cond_init(waitc);
    waiting=0;
    loaded=0;
    kbd_raw();
    if (thread_create_joinable(&loadth, loader, (intptr_t)fd))
        die("Cannot create thread");
    replay();
    pthread_cancel(loadth);
    thread_join(loadth);
    ttyrec_free(tr);
    kbd_restore();
    return 0;
}
