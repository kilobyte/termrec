#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "formats.h"
#include "vt100.h"
#include "synch.h"
#include "timeline.h"

struct tty_event tev_head, *tev_tail;
int tev_done;
static vt100 tev_vt;
static int nchunk;



void synch_init_wait(struct timeval *ts)
{
#ifdef HAVE_CTIME_R
    char buf[128];
#endif
    
    if (ts)
    {
        timeline_lock();
#ifdef HAVE_CTIME_R
        ctime_r(&ts->tv_sec, buf);
        vt100_printf(&tev_vt, "Recording started %s\n", buf);
#else
        vt100_printf(&tev_vt, "Recording started %s\n", ctime(&ts->tv_sec));
#endif
        timeline_unlock();
    }
}

void synch_wait(struct timeval *tv)
{
    if (tv->tv_sec>5)
        tv->tv_sec=5, tv->tv_usec=0;

    timeline_lock();
    if (tev_tail->data)
    {
        struct tty_event *tev_new;
        
        if (tev_tail==&tev_head)
        {
            vt100_free(tev_head.snapshot);
            vt100_copy(&tev_vt, tev_head.snapshot);
            nchunk=0;
        }
        else
            if (nchunk>65536)	/* Do a snapshot every 64KB of data */
            {
                if (!(tev_tail->snapshot=malloc(sizeof(vt100))))
                    goto fail;
                if (!vt100_copy(&tev_vt, tev_tail->snapshot))
                    goto fail_snapshot;
                nchunk=0;
            }
        tev_new=malloc(sizeof(struct tty_event));
        if (!tev_new)
            goto fail;
        memset(tev_new, 0, sizeof(struct tty_event));
        tev_new->t=tev_tail->t;
        tev_tail->next=tev_new;
        tev_tail=tev_new;
    }
    tadd(&tev_tail->t, tv);
    timeline_unlock();
    return;
fail_snapshot:
    vt100_free(tev_tail->snapshot);
fail:
    tev_done=1;
    timeline_unlock();
}

void synch_print(char *buf, int len)
{
    char *sp;

    timeline_lock();
    if (tev_tail->data)
        sp=realloc(tev_tail->data, tev_tail->len+len);
    else
        sp=malloc(len);
    if (!sp)
    {
        tev_done=1;
        timeline_unlock();
        return;
    }
    tev_tail->data=sp;
    memcpy(sp+tev_tail->len, buf, len);
    tev_tail->len+=len;
    vt100_write(&tev_vt, buf, len);
    nchunk+=len;
    timeline_unlock();
}

void timeline_init()
{
    memset(&tev_head, 0, sizeof(struct tty_event));
    memset(&tev_vt, 0, sizeof(vt100));
    tev_tail=&tev_head;
    tev_done=0;
}

void timeline_clear()
{
    struct tty_event *tc;

    tev_tail=&tev_head;
    while(tev_tail)
    {
        tc=tev_tail;
        tev_tail=tev_tail->next;
        free(tc->data);
        if (tc->snapshot)
        {
            vt100_free(tc->snapshot);
            free(tc->snapshot);
        }
        if (tc!=&tev_head)
            free(tc);
    }
    
    vt100_free(&tev_vt);
    memset(&tev_head, 0, sizeof(struct tty_event));
    tev_tail=&tev_head;
    tev_done=0;
    vt100_init(&tev_vt);
    vt100_resize(&tev_vt, 80, 25);
    vt100_printf(&tev_vt, "\e[36mTermplay v\e[1m"PACKAGE_VERSION"\e[0m\n\n");
    tev_head.snapshot=malloc(sizeof(vt100));
    vt100_copy(&tev_vt, tev_head.snapshot);
    nchunk=0;
}
