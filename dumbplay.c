#include "config.h"
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include "utils.h"
#include "formats.h"
#include "stream_in.h"
#include "synch.h"

FILE *play_f;

struct timeval t0,tw;
int speed;

void synch_init_wait(struct timeval *ts)
{
//    gettimeofday(&t0, 0);
//    tw=t0;
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
    
    fprintf(stderr, "Waiting for %lu.%06lu\n", tv->tv_sec, tv->tv_usec);
    do
    {
        gettimeofday(&tc, 0);
        
        fds=1;
        select(1, (fd_set*)&fds, 0, 0, tv);
        if (fds)
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
    }while(fds);
}

void synch_print(char *buf, int len)
{
    write(1, buf, len);
}

int main()
{
    if (!(play_f=stream_open("out.ttyrec")))
    {
        fprintf(stderr, "Can't open out.ttyrec\n");
        exit(1);
    }
    playfile(0);
    return 0;
}
