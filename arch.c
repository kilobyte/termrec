#include "arch.h"
/* Everything below is OS-dependant */

#ifdef WIN32

static DWORD tm;
int speed;
HANDLE pth;

void synch_init_wait()
{
    tm=GetTickCount();
}

void synch_wait(struct timeval *tv)
{
    DWORD tn,tw;

    // FIXME: round-up errors    
    tn=tm;
    tm=GetTickCount();
    tn=tm-tn;
    
    tw=tv->tv_sec*1000+tv->tv_usec/1000;
/*
    printf("Elapsed: %u, delay: %u.  Waiting %u.\n", tn, tw, tw-tn);
*/
    if (tw>5000)
        tw=5000;	// FIXME
    tw-=tn;
    tn=speed;
    if (tn)
        tw=(tw>0)?tw*1000/tn:0;
    else
        tw=INFINITE;
    if (WaitForSingleObject(changes, tw)==WAIT_OBJECT_0)
    {
        if (cancel)
            ExitThread(0);
        ReleaseMutex(changes);
    }
    tm=GetTickCount();
}

void synch_speed(int sp)
{
    speed=sp;
    ReleaseMutex(changes);
    WaitForSingleObject(changes, INFINITE);
}

void synch_seek(size_t where)
{
    cancel=1;
    ReleaseMutex(changes);
    WaitForSingleObject(changes, INFINITE);
}

LPTHREAD_START_ROUTINE playfile();

void synch_start(size_t where, int arg)
{
    pth=CreateThread(0, 0, (LPTHREAD_START_ROUTINE)playfile, 0, 0, (LPDWORD)arg);
}
#endif
