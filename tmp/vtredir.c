#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "vt100.h"

struct vtvt_data
{
    int attr;
    int cx,cy;
};

#define DATA ((struct vtvt_data*)(vt->l_data))

static inline void setattr(vt100 vt, int attr)
{
    if (DATA->attr!=attr)
    {
        DATA->attr=attr;
        printf("\e[0");
        if (attr&VT100_ATTR_BOLD)
            printf(";1");
        if (attr&VT100_ATTR_DIM)
            printf(";2");
        if (attr&VT100_ATTR_ITALIC)
            printf(";3");
        if (attr&VT100_ATTR_UNDERLINE)
            printf(";4");
        if (attr&VT100_ATTR_BLINK)
            printf(";5");
        if (attr&VT100_ATTR_INVERSE)
            printf(";7");
        if ((attr&0xff)!=0xff)
            printf(";3%u", attr&0xff);
        if ((attr&0xff00)!=0xff00)
            printf(";4%u", (attr&0xff00)>>8);
        printf("m");
    }
}


void vtvt_char(vt100 vt, int x, int y, ucs ch, int attr)
{
    setattr(vt, attr);
    printf("%c", ch);
}

void vtvt_cursor(vt100 vt, int x, int y)
{
    printf("\e[%d;%df", y+1, x+1);
}

void vtvt_dump(vt100 vt)
{
    int x,y;
    attrchar *ch;
    
    ch=vt->scr;
    for(y=0; y<vt->sy; y++)
    {
        printf("\e[%uf", y+1);
        for(x=0; x<vt->sx; x++)
        {
            vtvt_char(vt, x, y, ch->ch, ch->attr);
            ch++;
        }
    }
    vtvt_cursor(vt, vt->cx, vt->cy);
}

void vtvt_clear(vt100 vt, int x, int y, int len)
{
    setattr(vt, vt->attr);
    vtvt_cursor(vt, x, y);
    while(len--)
        printf(" ");	/* TODO */
    vtvt_cursor(vt, vt->cx, vt->cy);
}

void vtvt_scroll(vt100 vt, int nl)
{
    vtvt_dump(vt);
}

void vtvt_flag(vt100 vt, int f, int v)
{
}

void vtvt_resize(vt100 vt, int sx, int sy)
{
    printf("\e[8;%u;%u\n", sy, sx);
}

void vtvt_free(vt100 vt)
{
    free(vt->l_data);
}

void vtvt_init(vt100 vt)
{
    vt->l_data=malloc(sizeof(struct vtvt_data));
    vt->l_char=vtvt_char;
    vt->l_cursor=vtvt_cursor;
    vt->l_clear=vtvt_clear;
    vt->l_scroll=vtvt_scroll;
    vt->l_flag=vtvt_flag;
    vt->l_resize=vtvt_resize;
    vt->l_free=vtvt_free;
}


#define BS 512
int main()
{
    vt100 vt;
    char buf[BS];
    int l;
    
    vt = vt100_init(20, 5, 0, 1);
    vtvt_init(vt);
    
    while((l=read(0, buf, BS)))
    {
        if (l<0)
        {
            fprintf(stderr, "Error in read()\n");
            exit(1);
        }
        
        vt100_write(vt, buf, l);
    }
    return 0;
}
