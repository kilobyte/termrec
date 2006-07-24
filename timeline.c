#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "formats.h"
#include "vt100.h"
#include "synch.h"
#include "timeline.h"
#include "name.h"

/*******************/
/* gatherer thread */
/*******************/

struct tty_event tev_head, *tev_tail;
int tev_done;
static vt100 tev_vt;
static int nchunk;


void synch_init_wait(struct timeval *ts)
{
#if 0
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
#endif
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
    vt100_init(&tev_vt, defsx, defsy, 0);
    vt100_printf(&tev_vt, "\e[36mTermplay v\e[1m"PACKAGE_VERSION"\e[0m\n\n");
    tev_head.snapshot=malloc(sizeof(vt100));
    vt100_copy(&tev_vt, tev_head.snapshot);
    nchunk=0;
}


/*****************/
/* replay thread */
/*****************/

extern vt100 vt;
struct tty_event *tev_cur;
int play_state; /* 0: stopped, 1: paused, 2: playing, 3: waiting for input */
struct timeval t0, /* adjusted wall time at t=0 */
               tr; /* current point of replay */
int tev_curlp;	/* amount of data already played from the current block */
extern int speed;

void replay_pause()
{
    switch(play_state)
    {
    case 0:
    default:
    case 1:
        break;
    case 2:
        gettimeofday(&tr, 0);
        tsub(&tr, &t0);
        tmul(&tr, speed);
    case 3:
        play_state=1;
    }
}

void replay_resume()
{
    struct timeval t;

    switch(play_state)
    {
    case 0:
    default:
    case 1:
        gettimeofday(&t0, 0);
        t=tr;
        tdiv(&t, speed);
        tsub(&t0, &t);
        play_state=2;
        break;
    case 2:
    case 3:
        break;
    }
}

int replay_play(struct timeval *delay)
{ /* structures touched: tev, vt */
    switch(play_state)
    {
    case 0:
    default:
    case 1:
        return 0;
    case 2:
        gettimeofday(&tr, 0);
        tsub(&tr, &t0);
        tmul(&tr, speed);
        if (tev_cur->len>tev_curlp)
        {
            vt100_write(&vt, tev_cur->data+tev_curlp, tev_cur->len-tev_curlp);
            tev_curlp=tev_cur->len;
        }
        while (tev_cur->next && tcmp(tev_cur->next->t, tr)==-1)
        {
            tev_cur=tev_cur->next;
            if (tev_cur->data)
                vt100_write(&vt, tev_cur->data, tev_cur->len);
            tev_curlp=tev_cur->len;
        }
        if (tev_cur->next)
        {
            *delay=tev_cur->next->t;
            tsub(delay, &tr);
            tdiv(delay, speed);
            return 1;
        }
        play_state=tev_done?0:3;
    case 3:
        return 0;
    }
}

/* FIXME: actually use the snapshots */
void replay_seek()
{ /* structures touched: tev, vt */
    struct timeval t;

    tev_cur=&tev_head;

    vt100_free(&vt);
    vt100_copy(tev_head.snapshot, &vt);
    
    tev_curlp=tev_head.len;
    while (tev_cur->next && tcmp(tev_cur->next->t, tr)==-1)
    {
        tev_cur=tev_cur->next;
        if (tev_cur->data)
            vt100_write(&vt, tev_cur->data, tev_cur->len);
        tev_curlp=tev_cur->len;
    }
    t=tr;
    gettimeofday(&t0, 0);
    tdiv(&tr, speed);
    tsub(&t0, &tr);
}


void replay_export(FILE *record_f, int codec, struct timeval *selstart, struct timeval *selend)
{
    void* record_state;
    struct tty_event *tev;
    
    tev=&tev_head;
    while (tev && tcmp(tev->t, *selstart)==-1)
        tev=tev->next;
    if (!tev)
        return;
    
    record_state=(*rec[codec].init)(record_f, selstart);
  
    while (tev && tcmp(tev->t, *selend)<1)
    {
        if (tev->data)
            (*rec[codec].write)(record_f, record_state, &tev->t, tev->data, tev->len);
        tev=tev->next;
    }
    (*rec[codec].finish)(record_f, record_state);
    fclose(record_f);
}
