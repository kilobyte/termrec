#include "arch.h"
/* Everything below is OS-dependant */

#ifdef WIN32

static DWORD tm;

void init_wait()
{
    tm=GetTickCount();
}

void timed_wait(struct timeval *tv)
{
    DWORD tn,tw;
    
    tn=tm;
    tm=GetTickCount();
    tn=tm-tn;
    
    tw=tv->tv_sec*1000+tv->tv_usec/1000;
/*
    printf("Elapsed: %u, delay: %u.  Waiting %u.\n", tn, tw, tw-tn);
*/
    if (tw>5000)
        tw=5000;	// FIXME
    if (tn<tw)
        Sleep(tw-tn);
    tm=GetTickCount();
}

#endif
