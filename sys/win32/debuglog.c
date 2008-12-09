#include "config.h"
#include <stdio.h>
#include <stdarg.h>

#define DEBUGLOG "debuglog.txt"

static int once=0;

void debuglog(const char *txt, ...)
{
    va_list ap;
    FILE *f;
    
    f=fopen(DEBUGLOG, once?"a":"w");
    once=1;

    va_start(ap, txt);
    vfprintf(f, txt, ap);
    va_end(ap);
    fclose(f); 
}
