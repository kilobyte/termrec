#include <stdio.h>
#include <stdlib.h>
#include <ttyrec.h>
#include "sys/error.h"

static long long ax, bx;
static struct timespec a, b, c;

static void conv(long long x, struct timespec *t)
{
    t->tv_sec=x/1000000000;
    if (x<0 && x%1000000000)
        t->tv_sec--;
    t->tv_nsec=x-t->tv_sec*1000000000;
}

static void gen(void)
{
    ax=rand()%10000000000-2000000000;
    conv(ax, &a);
    bx=rand()%10000000000-2000000000;
    conv(bx, &b);
}

static void cmp(long long cx, const char *opname)
{
    conv(cx, &c);
    if (a.tv_sec!=c.tv_sec || labs(a.tv_nsec-c.tv_nsec)>1) // round-off error
        die("Mismatch in %s\n", opname);
}

int main(void)
{
    int i;

    for (i=0;i<1000;i++)
    {
        gen();
        //printf("%3d:%09u + %3d:%09u = ", a.tv_sec, a.tv_nsec, b.tv_sec, b.tv_nsec);
        tadd(a, b);
        //printf("%3d:%09u\n", a.tv_sec, a.tv_nsec);
        cmp(ax+bx, "tadd");
    }
    for (i=0;i<1000;i++)
    {
        gen();
        tsub(a, b);
        cmp(ax-bx, "tsub");
    }
    for (i=0;i<1000;i++)
    {
        gen();
        b.tv_nsec/=100000;
        //printf("%3d:%09u * %4d = ", a.tv_sec, a.tv_nsec, b.tv_nsec);
        tmul1000(a, b.tv_nsec);
        //printf("%3d:%09u vs %lld\n", a.tv_sec, a.tv_nsec, ax*b.tv_nsec/1000);
        cmp(ax*b.tv_nsec/1000, "tmul1000");
    }
    for (i=0;i<1000;i++)
    {
        gen();
        b.tv_nsec/=100000;
        if (b.tv_nsec<=0)
            continue;
        //printf("%3d:%09u / %4d = ", a.tv_sec, a.tv_nsec, b.tv_nsec);
        tdiv1000(a, b.tv_nsec);
        //printf("%3d:%09u vs %lld\n", a.tv_sec, a.tv_nsec, ax*(long long)1000/b.tv_nsec);
        cmp(ax*(long long)1000/b.tv_nsec, "tdiv1000");
    }
    return 0;
}
