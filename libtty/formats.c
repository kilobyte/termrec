/*
This file defines the "plugins".

There is no need to provide both recorder and a player.  Read-only or
write-only plugins are fine.
To create one, implement:


+ player:

void play_xxx(FILE *f,
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg, struct timeval *cont);
You are guaranteed to be running in a thread-equivalent on your own.
    *f
      is the file you're reading from, don't expect it to be seekable.
    void synch_init_wait(struct timeval *ts, void *arg);
      you may call this to mark the starting time of the stream.  It's purely
      optional, and its only use is to mention the date of the recording, if
      known.
    void synch_wait(struct timeval *tv, void *arg);
      call this to convey timing information.  The value given is the _delay_
      since the last call, not the absolute time.
    void synch_print(char *buf, int len, void *arg);
      call this to write the actual output.
    void *arg
      please pass it to each of the functions you call.


+ recorder:

void* record_xxx_init(FILE *f, struct timeval *tm);
Will be called exactly once per stream.
    *f
      is the file you're supposed to write the stream to.  Don't expect it to
      be seekable.
    *tm
      is the date of the recording, if known.  tm will be a null pointer if
      this information is not available -- in this case, all timing data will
      be relative to 0:0.
    You may allocate some memory for keeping track of the state.  In this case,
    return the pointer to that memory, it will be passed in every subsequent
    call.  It may be arbitrary data, and is never used outside your code.

void record_xxx(FILE *f, void* state, struct timeval *tm, char *buf, int len);
Will be called every time a new chunk of input comes.
    *f
      is the file we're recording to.
    *state
      is the pointer returned by record_xxx_init().
    *tm
      is the "current" time of the chunk, as an absolute value (_not_ the delay
      since the last call).  It usually will be measured as the real time since
      the Epoch, but in some cases, it may start at 0:0.
    *buf
      is the chunk to be written.  It won't be terminated with a \0.
    len
      is the length of the chunk.

void record_xxx_finish(FILE *f, void* state);
Will be called when the recording ends.
    *f
      is the file we're recording to.
    *state
      is the pointer returned by record_xxx_init().
    At this point, you need to free any memory you've allocated.

*/

#include "config.h"
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include "_stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "export.h"
#include "formats.h"
#include "compat.h"


/* ttyrec checks the byte sex on runtime, during _every_ swap.  Uh oh. */
#ifdef WORDS_BIGENDIAN
# define little_endian(x) ((uint32_t) ( \
    (((uint32_t) (x) & (uint32_t) 0x000000ffU) << 24) | \
    (((uint32_t) (x) & (uint32_t) 0x0000ff00U) <<  8) | \
    (((uint32_t) (x) & (uint32_t) 0x00ff0000U) >>  8) | \
    (((uint32_t) (x) & (uint32_t) 0xff000000U) >> 24)))
#else
# define little_endian(x) ((uint32_t)(x))
#endif

#define tadd(t, d)	{if (((t).tv_usec+=(d).tv_usec)>1000000)	\
                            (t).tv_usec-=1000000, (t).tv_sec++;		\
                         (t).tv_sec+=(d).tv_sec;			\
                        }
#define tsub(t, d)	{if ((signed)((t).tv_usec-=(d).tv_usec)<0)	\
                            (t).tv_usec+=1000000, (t).tv_sec--;		\
                         (t).tv_sec-=(d).tv_sec;			\
                        }

#define BUFFER_SIZE 32768


/********************/
/* format: baudrate */
/********************/

static void play_baudrate(FILE *f,
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg, struct timeval *cont)
{
    char buf[BUFFER_SIZE];
    size_t b;
    struct timeval tv;
    
    tv.tv_sec=0;
    tv.tv_usec=200000;
    while((b=fread(buf, 1, 60, f))>0)	/* 2400 baud */
    {
        synch_print(buf, b, arg);
        
        synch_wait(&tv, arg);
    }
}


static void* record_baudrate_init(FILE *f, struct timeval *tm)
{
    return 0;
}

static void record_baudrate(FILE *f, void* state, struct timeval *tm, char *buf, int len)
{
    fwrite(buf, 1, len, f);
}

static void record_baudrate_finish(FILE *f, void* state)
{
}


/***********************/
/* pseudo-format: live */
/***********************/

static void play_live(FILE *f,
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg, struct timeval *cont)
{
    struct timeval tv, tp, tm;
    char buf[BUFFER_SIZE];
    int len;
    
    if (cont)
        tp=*cont;
    else
    {
        gettimeofday(&tp, 0);
        synch_init_wait(&tp, arg);
    }
    
    /* using read() not fread(), we need unbuffered IO */
    while((len=read(fileno(f), buf, BUFFER_SIZE))>0)
    {
        gettimeofday(&tv, 0);
        tm=tv;
        tsub(tm, tp);
        synch_wait(&tm, arg);
        tp=tv;
        synch_print(buf, len, arg);
    }
}


static void* record_live_init(FILE *f, struct timeval *tm)
{
    struct timeval *tv;
    
    tv=malloc(sizeof(struct timeval));
    gettimeofday(tv, 0);
    tsub(*tv, *tm);
    
    return tv;
}

static void record_live(FILE *f, void* state, struct timeval *tm, char *buf, int len)
{
    struct timeval tv, wall;
    
    gettimeofday(&wall, 0);
    tv=*tm;
    tadd(tv, *((struct timeval*)state));
    tsub(tv, wall);
    if (tv.tv_sec>=0 && (tv.tv_sec || tv.tv_usec)) /* can't go back in time */
        select(0, 0, 0, 0, &tv);
    else
        tsub(*(struct timeval*)state, tv); /* move the origin by the (negative) time skipped */
    fwrite(buf, 1, len, f);
    fflush(f);
}

static void record_live_finish(FILE *f, void* state)
{
    free(state);
}


/******************/
/* format: ttyrec */
/******************/

struct ttyrec_header
{
    uint32_t sec;
    uint32_t usec;
    uint32_t len;
};


static void play_ttyrec(FILE *f,
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg, struct timeval *cont)
{
    char buf[BUFFER_SIZE];
    size_t b,w;
    struct timeval tv,tl;
    int first=1;
    struct ttyrec_header head;
    
    if (cont)
    {
        tv=*cont;
        first=0;
    }

    while(fread(&head, 1, sizeof(struct ttyrec_header), f)
      ==sizeof(struct ttyrec_header))
    {
        b=little_endian(head.len);
/*
        printf("b=%u, time=%i.%09i\n", b, little_endian(head.sec),
            little_endian(head.usec));
*/
        /* pre-read the first block to make the timing more accurate */
        if (b>BUFFER_SIZE)
            w=BUFFER_SIZE;
        else
            w=b;
        if (fread(buf, 1, w, f)<w)
            goto the_end;
        b-=w;
        
        if (first)
        {
            tv.tv_sec=little_endian(head.sec);
            tv.tv_usec=little_endian(head.usec);
            synch_init_wait(&tv, arg);
            first=0;
        }
        else
        {
            tl=tv;
            tv.tv_sec=little_endian(head.sec);
            tv.tv_usec=little_endian(head.usec);
            tl.tv_sec=tv.tv_sec-tl.tv_sec;
            if ((tl.tv_usec=tv.tv_usec-tl.tv_usec)<0)
            {
                tl.tv_usec+=1000000;
                tl.tv_sec--;
            }
            synch_wait(&tl, arg);
        }
        
        while(w>0)
        {
            synch_print(buf, w, arg);
            if (!b)
                break;
            if (b>BUFFER_SIZE)
                w=BUFFER_SIZE;
            else
                w=b;
            if (fread(buf, 1, w, f)<w)
                goto the_end;
            b-=w;
        }
    }
the_end:;
}


static void* record_ttyrec_init(FILE *f, struct timeval *tm)
{
    return 0;
}

static void record_ttyrec(FILE *f, void* state, struct timeval *tm, char *buf, int len)
{
    struct ttyrec_header h;
    
    h.sec=tm->tv_sec;
    h.usec=tm->tv_usec;
    h.len=len;
    fwrite(&h, 1, sizeof(h), f);
    fwrite(buf, 1, len, f);
}

static void record_ttyrec_finish(FILE *f, void* state)
{
}


/***********************/
/* format: nh_recorder */
/***********************/

static void play_nh_recorder(FILE *f,
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg, struct timeval *cont)
{
    char buf[BUFFER_SIZE];
    int b,i,i0;
    struct timeval tv;
    uint32_t t,tp;
    
    t=0;
    i=b=0;
    while((b=fread(buf+b-i, 1, BUFFER_SIZE-(b-i), f))>0)
    {
        i0=0;
        for(i=0;i<b;i++)
            if (!buf[i])
            {
                if (i0<i)
                    synch_print(buf+i0, i-i0, arg);
                if (i+4>b)	/* timestamp happened on a block boundary */
                {
                    if (b<5)	/* incomplete last timestamp */
                        return;
                    memmove(buf+i, buf, b-i);
                    goto block_end;
                }
                else
                {
                    tp=t;
                    t=little_endian(*(uint32_t*)(buf+i+1));
                    i0=i+=4;
                    tv.tv_sec=(t-tp)/100;
                    tv.tv_usec=(t-tp)%100*10000;
                    synch_wait(&tv, arg);
                }
            }
        if (i0<i)
            synch_print(buf+i0, i-i0, arg);
    block_end:;
    }
}

static void* record_nh_recorder_init(FILE *f, struct timeval *tm)
{
    struct timeval *tv;
    
    tv=malloc(sizeof(struct timeval));
    *tv=*tm;
    fwrite("\0\0\0\0\0", 1, 5, f);
    return tv;
}

static void record_nh_recorder(FILE *f, void* state, struct timeval *tm, char *buf, int len)
{
    int32_t i;
    i=(tm->tv_sec-((struct timeval*)state)->tv_sec)*100+
      (tm->tv_usec-((struct timeval*)state)->tv_usec)/10000;
    
    fwrite(buf, 1, len, f);
    fwrite("\0", 1, 1, f);
    fwrite(&i, 1, 4, f);
}

static void record_nh_recorder_finish(FILE *f, void* state)
{
    free(state);
}


/***********************/
/* pseudo-format: auto */
/***********************/

static void play_auto(FILE *f,
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg, struct timeval *cont)
{
    struct ttyrec_header tth;
    int len, got;
    char buf[BUFFER_SIZE];
    struct timeval tv;
    
    /* first, grab 12 bytes and see if it looks like a ttyrec header */
    got=0;
    do
    {
        /* must use unbuffered I/O here, to not spoil it for play_live() */
        if ((len=read(fileno(f), ((char*)&tth)+got, 12-got))<=0)
            return;
        got+=len;
    } while (got<12);
    if (little_endian(tth.sec)>=0 &&
        little_endian(tth.usec)>=0 && little_endian(tth.usec)<1000000 &&
        little_endian(tth.len)>0 && little_endian(tth.len)<65536)
    {
        tv.tv_sec=little_endian(tth.sec);
        tv.tv_usec=little_endian(tth.usec);
        synch_init_wait(&tv, arg);
        got=little_endian(tth.len);
        while(got>0)
        {
            if ((len=fread(buf, 1, (got>BUFFER_SIZE)?BUFFER_SIZE:got, f))<=0)
                return;
            synch_print(buf, len, arg);
            got-=len;
        }
        play_ttyrec(f, 0, synch_wait, synch_print, arg, &tv);
        return;
    }
    
    /* fall back to "live" */
    gettimeofday(&tv, 0);
    synch_init_wait(&tv, arg);
    synch_print((char*)&tth, 12, arg);
    play_live(f, 0, synch_wait, synch_print, arg, &tv);
}


/****************/
/* format: null */
/****************/

static void* record_null_init(FILE *f, struct timeval *tm)
{
    return 0;
}

static void record_null(FILE *f, void* state, struct timeval *tm, char *buf, int len)
{
}

static void record_null_finish(FILE *f, void* state)
{
}



void play_dosrecorder(FILE *f,
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg, struct timeval *cont);

recorder_info rec[]={
{"ansi",".txt",record_baudrate_init,record_baudrate,record_baudrate_finish},
{"ttyrec",".ttyrec",record_ttyrec_init,record_ttyrec,record_ttyrec_finish},
{"nh_recorder",".nh",record_nh_recorder_init,record_nh_recorder,record_nh_recorder_finish},
{"live", 0, record_live_init, record_live, record_live_finish},
{"null",0,record_null_init,record_null,record_null_finish},
{0, 0, 0, 0},
};

player_info play[]={
{"baudrate",".txt",play_baudrate},
{"ttyrec",".ttyrec",play_ttyrec},
{"nh_recorder",".nh",play_nh_recorder},
#if (defined HAVE_LIBZ) || (SHIPPED_LIBZ)
{"dosrecorder",".dm2",play_dosrecorder},
#endif
{"live",0,play_live},
{"auto",0,play_auto},
{0, 0},
};

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
int rec_n=ARRAYSIZE(rec)-1;
int play_n=ARRAYSIZE(play)-1;
