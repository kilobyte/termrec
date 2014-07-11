#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "tty.h"
#include "sys/ttysize.h"
#include "export.h"
#include "charsets.h"

struct vtvt_data
{
    FILE *f;
    int sx,sy;

    int attr;
    int cx,cy;
};

#define DATA ((struct vtvt_data*)(vt->l_data))
#define SX	DATA->sx
#define SY	DATA->sy
#define CX	DATA->cx
#define CY	DATA->cy


static inline void setattr(vt100 vt, int attr)
{
    if (DATA->attr!=attr)
    {
        DATA->attr=attr;
        fprintf(DATA->f, "\e[0");
        if (attr&VT100_ATTR_BOLD)
            fprintf(DATA->f, ";1");
        if (attr&VT100_ATTR_DIM)
            fprintf(DATA->f, ";2");
        if (attr&VT100_ATTR_ITALIC)
            fprintf(DATA->f, ";3");
        if (attr&VT100_ATTR_UNDERLINE)
            fprintf(DATA->f, ";4");
        if (attr&VT100_ATTR_BLINK)
            fprintf(DATA->f, ";5");
        if (attr&VT100_ATTR_INVERSE)
            fprintf(DATA->f, ";7");
        if ((attr&0xff)!=0x10)
            if ((attr&0xff)<0x10)
                fprintf(DATA->f, ";3%u", attr&0xff);
            else
                fprintf(DATA->f, ";38;5;%u", attr&0xff);
        if ((attr&0xff00)!=0x1000)
            if ((attr&0xff00)<0x1000)
                fprintf(DATA->f, ";4%u", (attr&0xff00)>>8);
            else
                fprintf(DATA->f, ";48;5;%u", (attr&0xff00)>>8);
        fprintf(DATA->f, "m");
    }
}

static void vtvt_cursor(vt100 vt, int x, int y)
{
    if (x==CX && y==CY)
        return;
    fprintf(DATA->f, "\e[%d;%df", y+1, x+1);
    CX=x;
    CY=y;
}

#define MAXVT100GRAPH 0x2666 // biggest codepoint is U+2666 BLACK DIAMOND
static char *vt100graph = 0;
static void build_vt100graph()
{
    int i;

    /* the reverse mapping Unicode->vt100_graphic takes a chunk of memory, so
       build it only on demand.  This won't happen most of the time. */
    if (!(vt100graph=malloc(MAXVT100GRAPH+1)))
        abort();
    memset(vt100graph, 0, MAXVT100GRAPH+1);
    for (i=32; i<127; i++)
        if (charset_vt100[i]<=MAXVT100GRAPH)
            vt100graph[charset_vt100[i]]=i;
}

static void vtvt_char(vt100 vt, int x, int y, ucs ch, int attr)
{
    if (x>=SX || y>SY)
        return;
    if (x!=CX || y!=CY)
        vtvt_cursor(vt, x, y);
    setattr(vt, attr);
    if (fprintf(DATA->f, "%lc", ch)<0)
    {
        if (vt->cp437)
        {
            if (!vt100graph)
                build_vt100graph();
            if (ch<=MAXVT100GRAPH && vt100graph[ch])
                fprintf(DATA->f, "\016%c\017", vt100graph[ch]);
            else
                fprintf(DATA->f, "?");
        }
        else
            fprintf(DATA->f, "?");
    }
    CX++;
}

export void vtvt_dump(vt100 vt)
{
    int x,y;
    attrchar *ch;

    ch=vt->scr;
    for (y=0; y<vt->sy; y++)
    {
        for (x=0; x<vt->sx; x++)
        {
            vtvt_char(vt, x, y, ch->ch, ch->attr);
            ch++;
        }
    }
    vtvt_cursor(vt, vt->cx, vt->cy);
}

static void vtvt_clear(vt100 vt, int x, int y, int len)
{
    int c;

    setattr(vt, vt->attr);
    if (x==0 && y==0 && len==SX*SY)
        fprintf(DATA->f, "\e[2J");
    else if (x==0 && y==0 && len==CY*SX+CX)
        fprintf(DATA->f, "\e[1J");
    else if (x==CX && y==CY && len==SX*SY-CY*SX-CX)
        fprintf(DATA->f, "\e[0J");
    else if (x==0 && y==CY && len==SX)
        fprintf(DATA->f, "\e[2K");
    else if (x==0 && y==CY && len==CX)
        fprintf(DATA->f, "\e[1K");
    else if (x==CX && y==CY && len==SX-CX)
        fprintf(DATA->f, "\e[0K");
    else
        while (len)
        {
            vtvt_cursor(vt, x, y);
            c=(len>SX-x)? SX-x : len;
            fprintf(DATA->f, "\e[%dX", c);
            len-=c;
            x=0;
            y++;
        }
    vtvt_cursor(vt, vt->cx, vt->cy);
}

static void vtvt_scroll(vt100 vt, int nl)
{
    if (nl>0)
        fprintf(DATA->f, "\e[0m\e[%d;%dr\e[%d;1f\e[%dM\e[r", vt->s1+1, vt->s2, vt->s1+1, nl);
    else if (nl<0)
        fprintf(DATA->f, "\e[0m\e[%d;%dr\e[%d;1f\e[%dL\e[r", vt->s1+1, vt->s2, vt->s1+1, -nl);
    CX=CY=-1;
    DATA->attr=0x1010;

//  vtvt_dump(vt);
}

static void vtvt_flag(vt100 vt, int f, int v)
{
    // TODO: invisible cursor
}

static void vtvt_resized(vt100 vt, int sx, int sy)
{
    fprintf(DATA->f, "\e[8;%u;%ut", sy, sx);
}

static void vtvt_flush(vt100 vt)
{
    fflush(DATA->f);
}

static void vtvt_free(vt100 vt)
{
    free(vt->l_data);
}

export void vtvt_resize(vt100 vt, int sx, int sy)
{
    SX=sx; SY=sy;
}

export void vtvt_attach(vt100 vt, FILE *f, int dump)
{
    vt->l_data=malloc(sizeof(struct vtvt_data));
    CX=CY=-1;
    DATA->attr=-1;
    DATA->f=f;

    SX=80; SY=25;
    get_tty_size(fileno(f), &SX, &SY);

    vt->l_char=vtvt_char;
    vt->l_cursor=vtvt_cursor;
    vt->l_clear=vtvt_clear;
    vt->l_scroll=vtvt_scroll;
    vt->l_flag=vtvt_flag;
    vt->l_resize=vtvt_resized;
    vt->l_flush=vtvt_flush;
    vt->l_free=vtvt_free;

    if (dump)
    {
        fprintf(DATA->f, "\ec");
        vtvt_dump(vt);
        fflush(stdout);
    }
}
