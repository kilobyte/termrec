#include <stdio.h>
#include "pty.h"

#define BS 65536

FILE *record_f;

int main()
{
    int ptym;
    char buf[BS];
    int r;

    if (!(ptym=run("bash",0,0)))
    {
        fprintf(stderr, "Couldn't allocaty pty.\n");
        return 1;
    }

    sleep(20);

    while((r=read(ptym, buf, BS))>0)
        write(1, buf, r);

    return 0;
}
