#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "_stdint.h"
#include "utils.h"


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

#ifdef HAVE_LANGINFO_H 
#include <locale.h> 
#include <langinfo.h> 

int is_utf8()
{
    setlocale(LC_CTYPE, "");
    return !strcmp(nl_langinfo(CODESET), "UTF-8");
}
#else 
int is_utf8()
{
    return 0; /* unlikely on musty pieces of cruft */
}
#endif 
