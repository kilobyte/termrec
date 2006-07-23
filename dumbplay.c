#include "config.h"
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include "utils.h"
#include "formats.h"
#include "stream.h"
#include "synch.h"

FILE *play_f;

struct timeval t0,tw;
int speed;
char *name;

void synch_init_wait(struct timeval *ts)
{
    gettimeofday(&t0, 0);
    tw=t0;
}

void synch_set_speed(int sp)
{
}

void synch_wait(struct timeval *tv)
{
    struct timeval tc;
    int fds;
    char k;

    if (tv->tv_sec>=5)
        tv->tv_sec=5, tv->tv_usec=0;
    
/*    fprintf(stderr, "Waiting for %lu.%06lu\n", tv->tv_sec, tv->tv_usec);*/
    do
    {
        gettimeofday(&tc, 0);
        
        fds=1;
/*      select(1, (fd_set*)&fds, 0, 0, tv);*/
        select(0, 0, 0, 0, tv);
        if (/*fds*/0)
        {
            if (read(0, &k, 1)>0)
                switch(k)
                {
                case '+':
                case 'f':
                    synch_set_speed(speed? speed*2 : 1);
                    break;
                case '-':
                case 's':
                    synch_set_speed(speed/2);
                    break;
                case '1':
                    synch_set_speed(1000);
                    break;
                case 'q':
                    exit(0);
                }
        }
    }while(/*fds*/0);
}

void synch_print(char *buf, int len)
{
    write(1, buf, len);
}

int main()
{
    int codec;

    name="out.ttyrec";
    if (!(play_f=stream_open(fopen(name, "rb"), name, "rb", decompressors, 0)))
        error("Can't open %s\n", name);
    codec=codec_from_ext_play(name);
    if (codec==-1)
        codec=0;
    play[codec].play(play_f);
    return 0;
}
