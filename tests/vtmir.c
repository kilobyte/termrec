#include "config.h"
#include <vt100.h>
#include <stdio.h>
#include "sys/threads.h"
#include "sys/compat.h"
#include "sys/error.h"
#include "_stdint.h"


vt100 vt1, vt2;

#define BUFFER_SIZE 128
void copier(void *args)
{
    int fd=(intptr_t)args;
    char buf[BUFFER_SIZE];
    int len;
    
    while((len=read(fd, buf, BUFFER_SIZE))>0)
        vt100_write(vt2, buf, len);
}

void dump(vt100 vt)
{
    int x,y;
    
    for(y=0;y<vt->sy;y++)
    {
        for (x=0;x<vt->sx;x++)
            printf("%lc", vt->scr[y*vt->sx+x].ch);
        printf("|\n");
    }
    printf("===================='\n");
}

#define BUF2 42
int main()
{
    thread_t cop;
    int p[2];
    FILE *f;
    char buf[BUF2];
    int len, i;
    
    vt1=vt100_init(20, 5, 0, 1);
    vt2=vt100_init(20, 5, 0, 1);
    if (pipe(p))
        die("pipe()");
    if (thread_create_joinable(&cop, copier, (void*)(intptr_t)p[0]))
        die("thread creation");
    f=fdopen(p[1], "w");
    vtvt_attach(vt1, f);
    
    while((len=read(0, buf, BUF2))>0)
        vt100_write(vt1, buf, len);
    fclose(f);
    thread_join(cop);
    
    for(i=0; i<vt1->sx*vt1->sy; i++)
        if (vt1->scr[i].ch!=vt2->scr[i].ch || vt1->scr[i].attr!=vt2->scr[i].attr)
        {
            printf("Mismatch!  Dumps:\n");
            dump(vt1);
            dump(vt2);
            return 1;
        }
    return 0;
}
