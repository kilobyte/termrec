#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "sys/threads.h"
#include "utils.h"
#include "vt100.h"
#include "export.h"
#include "files.h"

struct ttyrec_frame
{
    struct timeval t;
    int len;
    char *data;
    vt100 snapshot;
    struct ttyrec_frame *next;
};

typedef struct ttyrec
{
    struct ttyrec_frame *tev_head, *tev_tail;
    vt100 tev_vt;
    int nchunk;
    mutex_t lock;
} *ttyrec;

#define SNAPSHOT_CHUNK 65536

static void ttyrec_lock(ttyrec tr);
static void ttyrec_unlock(ttyrec tr);

#define tr ((ttyrec)arg)
static void synch_init_wait(struct timeval *ts, void *arg)
{
    tr->tev_head->t=*ts;
}

static void synch_wait(struct timeval *tv, void *arg)
{
    if (tv->tv_sec>=5 || tv->tv_sec<0)
        tv->tv_sec=5, tv->tv_usec=0;
    
    ttyrec_lock(tr);
    if (tr->tev_tail->data)
    {
        struct ttyrec_frame *tev_new;
        
        if (tr->nchunk>=SNAPSHOT_CHUNK)	/* Do a snapshot every 64KB of data */
        {
            if (!(tr->tev_head->snapshot=vt100_copy(tr->tev_vt)))
                goto fail_snapshot;
            tr->nchunk=0;
        }
        tev_new=malloc(sizeof(struct ttyrec_frame));
        if (!tev_new)
            goto fail;
        memset(tev_new, 0, sizeof(struct ttyrec_frame));
        tev_new->t=tr->tev_tail->t;
        tr->tev_tail->next=tev_new;
        tr->tev_tail=tev_new;
    }
    tadd(&tr->tev_tail->t, tv);
    ttyrec_unlock(tr);
    return;
fail:
    vt100_free(tr->tev_tail->snapshot);
fail_snapshot:
    ttyrec_unlock(tr);
}

static void synch_print(char *buf, int len, void *arg)
{
    char *sp;
    
    ttyrec_lock(tr);
    if (tr->tev_tail->data)
        sp=realloc(tr->tev_tail->data, tr->tev_tail->len+len);
    else
        sp=malloc(len);
    if (!sp)
    {
        ttyrec_unlock(tr);
        return;
    }
    tr->tev_tail->data=sp;
    memcpy(sp+tr->tev_tail->len, buf, len);
    tr->tev_tail->len+=len;
    vt100_write(tr->tev_vt, buf, len);
    tr->nchunk+=len;
    ttyrec_unlock(tr);
}
#undef tr


export ttyrec ttyrec_init(vt100 vt)
{
    ttyrec tr = malloc(sizeof(struct ttyrec));
    tr->tev_head = malloc(sizeof(struct ttyrec_frame));
    memset(tr->tev_head, 0, sizeof(struct ttyrec_frame));
    tr->tev_tail=tr->tev_head;
    tr->nchunk=SNAPSHOT_CHUNK;
    mutex_init(tr->lock);
    
    tr->tev_vt = vt? vt : vt100_init(80, 25, 1, 0);
    
    return tr;
}


export void ttyrec_free(ttyrec tr)
{
    struct ttyrec_frame *tev_tail, *tc;
    
    if (!tr)
        return;
    
    ttyrec_lock(tr);
    tev_tail=tr->tev_head;
    while(tev_tail)
    {
        tc=tev_tail;
        tev_tail=tev_tail->next;
        free(tc->data);
        if (tc->snapshot)
            vt100_free(tc->snapshot);
        free(tc);
    }
    vt100_free(tr->tev_vt);
    mutex_destroy(tr->lock);
    free(tr);
}


export ttyrec ttyrec_load(int fd, char *format, char *filename, vt100 vt)
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


static void ttyrec_lock(ttyrec tr)
{
    mutex_lock(tr->lock);
}

static void ttyrec_unlock(ttyrec tr)
{
    mutex_unlock(tr->lock);
}

export struct ttyrec_frame* ttyrec_seek(ttyrec tr, struct timeval *t, vt100 *vt)
{
    struct ttyrec_frame *tfv, *tfs;
    
    if (!tr)
        return 0;
    
    tfv = tr->tev_head;
    tfs = tfv->snapshot ? tfv : 0;
    
    if (t)
        while(tfv->next && tcmp(tfv->next->t, *t)<=0)
        {
            tfv=tfv->next;
            if (tfv->snapshot)
                tfs=tfv;
        }
    
    if (vt)
    {
        vt100_free(*vt);
        if (tfs)
            *vt=vt100_copy(tfs->snapshot);
        else
        {
            tfs=tr->tev_head;
            *vt=vt100_init(80, 25, 1, 0);
        }
        if (!*vt)
            return 0;
        while (tfs!=tfv)
        {
            if (tfs->data)
                vt100_write(*vt, tfs->data, tfs->len);
            tfs = tfs->next;
        } while (tfs!=tfv);
        if (tfs->data)
            vt100_write(*vt, tfs->data, tfs->len);
    }
    
    return tfv;
}


export struct ttyrec_frame* ttyrec_next_frame(ttyrec tr, struct ttyrec_frame *tfv)
{
    if (!tfv)
        return 0;
    return tfv->next;
}


export void ttyrec_add_frame(ttyrec tr, struct timeval *delay, char *data, int len)
{
    if (delay)
        synch_wait(delay, tr);
    synch_print(data, len, tr);
}


export int ttyrec_save(ttyrec tr, int fd, char *format, char *filename, struct timeval *selstart, struct timeval *selend)
{
    struct ttyrec_frame *fr;
    recorder rec;
    
    fr=ttyrec_seek(tr, selstart, 0);
    if (!(rec=ttyrec_w_open(fd, format, filename, &fr->t)))
        return 0;
    while(fr)
    {
        if (selend && tcmp(fr->t, *selend)>0)
            break;
        ttyrec_w_write(rec, &fr->t, fr->data, fr->len);
        fr=ttyrec_next_frame(tr, fr);
    }
    return ttyrec_w_close(rec);
}
