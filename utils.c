#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "_stdint.h"
#include "utils.h"

#ifndef HAVE_ASPRINTF
int asprintf(char **sptr, const char *fmt, ...)
{
    /* Guess we need no more than 100 bytes. */
    int n, size = 64;
    char *p;
    va_list ap;
    if ((p = malloc (size)) == NULL)
    {
        *sptr=0;        
        return -1;
    }
    while (1)
    {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        n = vsnprintf (p, size, fmt, ap);
        va_end(ap);
        /* If that worked, return the string. */
        if (n > -1 && n < size)
        {
            *sptr=p;
            return n;
        }
        /* Else try again with more space. */
        if (n > -1)    /* glibc 2.1 */
            size = n+1; /* precisely what is needed */
        else           /* glibc 2.0 */
            size *= 2;  /* twice the old size */
        if ((p = realloc (p, size)) == NULL)
        {
            *sptr=0;
            return -1;
        }
    }
}
#endif

void tadd(struct timeval *t, struct timeval *d)
{
    if ((t->tv_usec+=d->tv_usec)>1000000)
        t->tv_usec-=1000000, t->tv_sec++;
    t->tv_sec+=d->tv_sec;    
}

void tsub(struct timeval *t, struct timeval *d)
{
    if ((signed)(t->tv_usec-=d->tv_usec)<0)
        t->tv_usec+=1000000, t->tv_sec--;
    t->tv_sec-=d->tv_sec;    
}

void tmul(struct timeval *t, int m)
{
    uint64_t v;
    
    v=((uint64_t)t->tv_usec)*m/1000+((uint64_t)t->tv_sec)*m*1000;
    t->tv_usec=v%1000000;
    t->tv_sec=v/1000000;
}

void tdiv(struct timeval *t, int m)
{
    uint64_t v;
    
    m=1000000/m;
    v=((uint64_t)t->tv_usec)*m/1000+((uint64_t)t->tv_sec)*m*1000;
    t->tv_usec=v%1000000;
    t->tv_sec=v/1000000;
}

int match_suffix(char *txt, char *ext, int skip)
{
    int tl,el;
    
    tl=strlen(txt);
    el=strlen(ext);
    if (tl-el-skip<0)
        return 0;
    txt+=tl-el-skip;
    return (!strncasecmp(txt, ext, el));
}

#ifndef IS_WIN32
void error(const char *txt, ...)
{
    va_list ap;
    
    va_start(ap, txt);
    vfprintf(stderr, txt, ap);
    exit(1);
    va_end(ap); /* make ugly compilers happy */
}
#else
# include "winutils.c"
#endif
