#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
{	// FIXME: this relies on tv_* being signed.
    if ((t->tv_usec-=d->tv_usec)<0)
        t->tv_usec+=1000000, t->tv_sec--;
    t->tv_sec-=d->tv_sec;    
}

#ifdef IS_WIN32
# include "winutils.c"
#endif
