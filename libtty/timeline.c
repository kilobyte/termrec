#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "error.h"
#include "tty.h"

typedef struct {} *recorder;

struct ttyrec_frame
{
    struct timespec t;
    int len;
    char *data;
    tty snapshot;
    struct ttyrec_frame *next;
};
typedef struct ttyrec_frame *ttyrec_frame;

typedef struct ttyrec
{
    struct ttyrec_frame *tev_head, *tev_tail;
    tty tev_vt;
    int nchunk;
    struct timespec ndelay;
} *ttyrec;
#define _TTYREC_H_NO_TYPES
#include "ttyrec.h"
#include "export.h"

#define SNAPSHOT_CHUNK 65536

#define tr ((ttyrec)arg)
static void synch_init_wait(const struct timespec *ts, void *arg)
{
    tr->tev_head->t=*ts;
}

static const struct timespec maxd = {5,0};

static void synch_wait(const struct timespec *tv, void *arg)
{
    if (tv->tv_sec>=maxd.tv_sec || tv->tv_sec<0)
        tadd(tr->ndelay, maxd);
    else
        tadd(tr->ndelay, *tv);
}

static void synch_print(const char *buf, int len, void *arg)
{
    struct ttyrec_frame *nf;

    if (!(nf=malloc(sizeof(struct ttyrec_frame))))
        return;
    if (!(nf->data=malloc(len)))
    {
        free(nf);
        return;
    }
    nf->len=len;
    memcpy(nf->data, buf, len);
    nf->t=tr->tev_tail->t;
    tadd(nf->t, tr->ndelay);
    tr->ndelay.tv_sec=tr->ndelay.tv_nsec=0;
    nf->snapshot=0;
    nf->next=0;
    tr->tev_tail->next=nf;
    tr->tev_tail=nf;
    tty_write(tr->tev_vt, buf, len);
    if ((tr->nchunk+=len)>=SNAPSHOT_CHUNK) // do a snapshot every 64KB of data
    {
        nf->snapshot=tty_copy(tr->tev_vt);
        tr->nchunk=0;
    }
}
#undef tr


export ttyrec ttyrec_init(tty vt)
{
    ttyrec tr = malloc(sizeof(struct ttyrec));
    if (!tr)
        return NULL;
    memset(tr, 0, sizeof(struct ttyrec));
    tr->tev_head = malloc(sizeof(struct ttyrec_frame));
    memset(tr->tev_head, 0, sizeof(struct ttyrec_frame));
    tr->tev_tail=tr->tev_head;
    tr->nchunk=SNAPSHOT_CHUNK;

    tr->tev_vt = vt? vt : tty_init(80, 25, 1);
    tr->tev_head->snapshot = tty_copy(tr->tev_vt);

    return tr;
}


export void ttyrec_free(ttyrec tr)
{
    struct ttyrec_frame *tev_tail, *tc;

    if (!tr)
        return;

    tev_tail=tr->tev_head;
    while (tev_tail)
    {
        tc=tev_tail;
        tev_tail=tev_tail->next;
        free(tc->data);
        if (tc->snapshot)
            tty_free(tc->snapshot);
        free(tc);
    }
    tty_free(tr->tev_vt);
    free(tr);
}


export ttyrec ttyrec_load(int fd, const char *format, const char *filename, tty vt)
{
    ttyrec tr;

    if (!(tr=ttyrec_init(vt)))
        return 0;
    if (!ttyrec_r_play(fd, format, filename, synch_init_wait, synch_wait, synch_print, tr))
    {
        ttyrec_free(tr);
        return 0;
    }
    return tr;
}


export struct ttyrec_frame* ttyrec_seek(const ttyrec tr, const struct timespec *t, tty *vt)
{
    struct ttyrec_frame *tfv, *tfs;

    if (!tr)
        return 0;

    tfv = tr->tev_head;
    tfs = 0;

    if (t)
        while (tfv->next && tcmp(tfv->next->t, *t)<=0)
        {
            tfv=tfv->next;
            if (tfv->snapshot)
                tfs=tfv;
        }
    else
        if (tfv->next)
            tfv=tfv->next;

    if (tfv->snapshot)
        tfs=tfv;

    if (vt)
    {
        tty_free(*vt);
        if (tfs)
            *vt=tty_copy(tfs->snapshot);
        else
        {
            tfs=tr->tev_head;
            *vt=tty_init(80, 25, 1);
        }
        if (!*vt)
            return 0;
        while (tfs!=tfv)
        {
            tfs = tfs->next;
            if (tfs->data)
                tty_write(*vt, tfs->data, tfs->len);
        };
    }

    return tfv;
}


export struct ttyrec_frame* ttyrec_next_frame(ttyrec tr, struct ttyrec_frame *tfv)
{
    if (!tfv)
        return 0;
    return tfv->next;
}


export void ttyrec_add_frame(ttyrec tr, const struct timespec *delay, const char *data, int len)
{
    if (delay)
        synch_wait(delay, tr);
    synch_print(data, len, tr);
}


export int ttyrec_save(ttyrec tr, int fd, const char *format, const char *filename,
                       const struct timespec *selstart, const struct timespec *selend)
{
    struct ttyrec_frame *fr;
    recorder rec;

    fr=ttyrec_seek(tr, selstart, 0);
    if (!(rec=ttyrec_w_open(fd, format, filename, &fr->t)))
        return 0;
    while (fr)
    {
        if (selend && tcmp(fr->t, *selend)>0)
            break;
        ttyrec_w_write(rec, &fr->t, fr->data, fr->len);
        fr=ttyrec_next_frame(tr, fr);
    }
    return ttyrec_w_close(rec);
}
