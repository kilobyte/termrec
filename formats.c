#include <stdio.h>
#include "arch.h"
#include "vt100.h"

extern FILE *f;
extern vt100 vt;


#define BUFFER_SIZE 32768

void play_baudrate()
{
    char buf[BUFFER_SIZE];
    size_t b;
    struct timeval tv;
    
    synch_init_wait();
    
    tv.tv_sec=0;
    tv.tv_usec=200000;
    while((b=fread(buf, 1, BUFFER_SIZE, f))>0)
    {
        VT100_LOCK;
        vt100_write(&vt, buf, b);
        VT100_UNLOCK;
        
        synch_wait(&tv);
    }
}


struct ttyrec_header
{
    uint32 sec;
    uint32 usec;
    uint32 len;
};


void play_ttyrec()
{
    char buf[BUFFER_SIZE];
    size_t b,w;
    struct timeval tv,tl;
    int first=1;
    struct ttyrec_header head;

    while(fread(&head, 1, sizeof(struct ttyrec_header), f)
      ==sizeof(struct ttyrec_header))
    {
        b=to_little_endian(head.len);
/*
        printf("b=%u, time=%i.%09i\n", b, to_little_endian(head.sec),
            to_little_endian(head.usec));
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
            tv.tv_sec=to_little_endian(head.sec);
            tv.tv_usec=to_little_endian(head.usec);
            synch_init_wait();
            first=0;
        }
        else
        {
            tl=tv;
            tv.tv_sec=to_little_endian(head.sec);
            tv.tv_usec=to_little_endian(head.usec);
            tl.tv_sec=tv.tv_sec-tl.tv_sec;
            if ((tl.tv_usec=tv.tv_usec-tl.tv_usec)<0)
            {
                tl.tv_usec+=1000000;
                tl.tv_sec--;
            }
            synch_wait(&tl);
            
            while(w>0)
            {
                VT100_LOCK;
                vt100_write(&vt, buf, w);
                VT100_UNLOCK;
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
    }
the_end:
    VT100_LOCK;
    vt100_printf(&vt, "\e[0mEnd of recording.");
    VT100_UNLOCK;
}


void play_nh_recorder()
{
    char buf[BUFFER_SIZE];
    int b,i,i0;
    struct timeval tv;
    uint32 t,tp;
    
    t=0;
    i=b=0;
    while((b=fread(buf+b-i, 1, BUFFER_SIZE-(b-i), f))>0)
    {
        i0=0;
        for(i=0;i<b;i++)
            if (!buf[i])
            {
                if (i0<i)
                {
                    VT100_LOCK;
                    vt100_write(&vt, buf+i0, i-i0);
                    VT100_UNLOCK;
                }
                if (i+4>b)	// timestamp happened on a block boundary
                {
                    if (b<5)	// incomplete last timestamp
                        return;
                    memmove(buf+i, buf, b-i);
                    goto block_end;
                }
                else
                {
                    tp=t;
                    t=to_little_endian(*(uint32*)(buf+i+1));
                    i0=i+=4;
                    tv.tv_sec=(t-tp)/100;
                    tv.tv_usec=(t-tp)%100*10000;
                    synch_wait(&tv);
                }
            }
        if (i0<i)
        {
            VT100_LOCK;
            vt100_write(&vt, buf+i0, i-i0);
            VT100_UNLOCK;
        }
    block_end:;
    }
    VT100_LOCK;
    vt100_printf(&vt, "\e[0mEnd of recording.");
    VT100_UNLOCK;
}


void playfile(int arg)
{
    play_ttyrec();
}
