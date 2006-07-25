#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "vt100.h"

struct rl_data
{
    int attr;
    int cx,cy;
};

#define DATA ((struct rl_data*)(vt->rl_data))

void rl_char(vt100 *vt, int x, int y, ucs ch, int attr)
{
    printf("%c", ch);
}

void rl_cursor(vt100 *vt, int x, int y)
{
    printf("\e[%d;%df", y+1, x+1);
}

void rl_clear(vt100 *vt, int x, int y, int len)
{
    rl_cursor(vt, x, y);
    while(len--)
        printf(" ");
    rl_cursor(vt, vt->cx, vt->cy);
}

void rl_scroll(vt100 *vt, int nl)
{
}

void rl_flag(vt100 *vt, int f, int v)
{
}

void rl_resize(vt100 *vt, int sx, int sy)
{
    printf("\e[8;%u;%u\n", sy, sx);
}

void rl_free(vt100 *vt)
{
    free(vt->l_data);
}

void rl_init(vt100 *vt)
{
    vt->l_data=malloc(sizeof(struct rl_data));
    vt->l_char=rl_char;
    vt->l_cursor=rl_cursor;
    vt->l_clear=rl_clear;
    vt->l_scroll=rl_scroll;
    vt->l_flag=rl_flag;
    vt->l_resize=rl_resize;
    vt->l_free=rl_free;
}


#define BS 512
int main()
{
    vt100 vt;
    char buf[BS];
    int l;
    
    vt100_init(&vt);
    vt100_resize(&vt, 20, 5);
    rl_init(&vt);
    
    while((l=read(0, buf, BS)))
    {
        if (l<0)
        {
            fprintf(stderr, "Error in read()\n");
            exit(1);
        }
        
        vt100_write(&vt, buf, l);
    }
    return 0;
}
