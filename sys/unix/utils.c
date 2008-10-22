#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void error(const char *txt, ...)
{
    va_list ap;

    va_start(ap, txt);
    vfprintf(stderr, txt, ap);
    exit(1);
    va_end(ap);
}
