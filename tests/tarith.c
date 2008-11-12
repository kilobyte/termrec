#include <stdio.h>
#include <stdlib.h>
#include <ttyrec.h>
#include "sys/error.h"

static long long ax, bx;
static struct timeval a, b, c;

static void conv(long long x, struct timeval *t)
{
    t->tv_sec=x/1000000;
    if (x<0 && x%1000000)
        t->tv_sec--;
    t->tv_usec=x-t->tv_sec*1000000;
}

static void gen()
{
    ax=random()%10000000-2000000;
    conv(ax, &a);
    bx=random()%10000000-2000000;
    conv(bx, &b);
}

static void cmp(long long cx, char *opname)
{
    conv(cx, &c);
    if (a.tv_sec!=c.tv_sec || abs(a.tv_usec-c.tv_usec)>1) /* round-off error */
        error("Mismatch in %s\n", opname);
}

int main()
{
    int i;
    
    for(i=0;i<1000;i++)
    {
        gen();
        /*printf("%3d:%09u + %3d:%09u = ", a.tv_sec, a.tv_usec, b.tv_sec, b.tv_usec);*/
        tadd(a, b);
        /*printf("%3d:%09u\n", a.tv_sec, a.tv_usec);*/
        cmp(ax+bx, "tadd");
    }
    for(i=0;i<1000;i++)
    {
        gen();
        tsub(a, b);
        cmp(ax-bx, "tsub");
    }
    for(i=0;i<1000;i++)
    {
        gen();
        b.tv_usec/=100;
        /*printf("%3d:%06u * %4d = ", a.tv_sec, a.tv_usec, b.tv_usec);*/
        tmul1000(a, b.tv_usec);
        /*printf("%3d:%06u vs %lld\n", a.tv_sec, a.tv_usec, ax*b.tv_usec/1000);*/
        cmp(ax*b.tv_usec/1000, "tmul1000");
    }
    for(i=0;i<1000;i++)
    {
        gen();
        b.tv_usec/=100;
        if (b.tv_usec<=0)
            continue;
        /*printf("%3d:%06u / %4d = ", a.tv_sec, a.tv_usec, b.tv_usec);*/
        tdiv1000(a, b.tv_usec);
        /*printf("%3d:%06u vs %lld\n", a.tv_sec, a.tv_usec, ax*(long long)1000/b.tv_usec);*/
        cmp(ax*(long long)1000/b.tv_usec, "tdiv1000");
    }
    return 0;
}
