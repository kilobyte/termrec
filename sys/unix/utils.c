#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

void die(const char *txt, ...)
{
    va_list ap;
    char *nl;
    int err=errno;
    
    va_start(ap, txt);
    vfprintf(stderr, txt, ap);
    va_end(ap);
    
    nl=strrchr(txt, '\n');
    if (!nl || nl[1])
    {
        if (err)
            fprintf(stderr, ": %s\n", strerror(err));
        else
            fprintf(stderr, ": unknown error\n");
    }
    
    exit(1);
}
