#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "vt100.h"
#include "sys/ttysize.h"
#include "export.h"

struct vtvt_data
{
    FILE *tty;
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
        fprintf(DATA->tty, "\e[0");
        if (attr&VT100_ATTR_BOLD)
            fprintf(DATA->tty, ";1");
        if (attr&VT100_ATTR_DIM)
            fprintf(DATA->tty, ";2");
        if (attr&VT100_ATTR_ITALIC)
            fprintf(DATA->tty, ";3");
        if (attr&VT100_ATTR_UNDERLINE)
            fprintf(DATA->tty, ";4");
        if (attr&VT100_ATTR_BLINK)
            fprintf(DATA->tty, ";5");
        if (attr&VT100_ATTR_INVERSE)
            fprintf(DATA->tty, ";7");
        if ((attr&0xff)!=0xff)
            fprintf(DATA->tty, ";3%u", attr&0xff);
        if ((attr&0xff00)!=0xff00)
            fprintf(DATA->tty, ";4%u", (attr&0xff00)>>8);
        fprintf(DATA->tty, "m");
    }
}

static void vtvt_cursor(vt100 vt, int x, int y)
{
    if (x==CX && y==CY)
        return;
    fprintf(DATA->tty, "\e[%d;%df", y+1, x+1);
    CX=x;
    CY=y;
}

static void vtvt_char(vt100 vt, int x, int y, ucs ch, int attr)
{
    if (x>=SX || y>SY)
        return;
    if (x!=CX || y!=CY)
        vtvt_cursor(vt, x, y);
    setattr(vt, attr);
    fprintf(DATA->tty, "%lc", ch);
    CX++;
}

export void vtvt_dump(vt100 vt)
{
    int x,y;
    attrchar *ch;
    
    ch=vt->scr;
    for(y=0; y<vt->sy; y++)
    {
        for(x=0; x<vt->sx; x++)
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
        fprintf(DATA->tty, "\e[2J");
    else if (x==0 && y==0 && len==CY*SX+CX)
        fprintf(DATA->tty, "\e[1J");
    else if (x==CX && y==CY && len==SX*SY-CY*SX-CX)
        fprintf(DATA->tty, "\e[0J");
    else if (x==0 && y==CY && len==SX)
        fprintf(DATA->tty, "\e[2K");
    else if (x==0 && y==CY && len==CX)
        fprintf(DATA->tty, "\e[1K");
    else if (x==CX && y==CY && len==SX-CX)
        fprintf(DATA->tty, "\e[0K");
    else
        while(len)
        {
            vtvt_cursor(vt, x, y);
            c=(len>SX-x)? SX-x : len;
            fprintf(DATA->tty, "\e[%dX", c);
            len-=c;
            x=0;
            y++;
        }
    vtvt_cursor(vt, vt->cx, vt->cy);
}

static void vtvt_scroll(vt100 vt, int nl)
{
    if (nl>0)
        fprintf(DATA->tty, "\e[0m\e[%d;%dr\e[%d;1f\e[%dM\e[r", vt->s1+1, vt->s2, vt->s1+1, nl);
    else if (nl<0)
        fprintf(DATA->tty, "\e[0m\e[%d;%dr\e[%d;1f\e[%dL\e[r", vt->s1+1, vt->s2, vt->s1+1, -nl);
    CX=CY=-1;
    DATA->attr=0xFFFF;

/*  vtvt_dump(vt); */
}

static void vtvt_flag(vt100 vt, int f, int v)
{
    /* TODO: invisible cursor */
}

static void vtvt_resized(vt100 vt, int sx, int sy)
{
    fprintf(DATA->tty, "\e[8;%u;%ut", sy, sx);
}

static void vtvt_flush(vt100 vt)
{
    fflush(DATA->tty);
}

static void vtvt_free(vt100 vt)
{
    free(vt->l_data);
}

export void vtvt_resize(vt100 vt, int sx, int sy)
{
    SX=sx; SY=sy;
}

export void vtvt_attach(vt100 vt, FILE *tty, int dump)
{
    is_utf8();
    vt->l_data=malloc(sizeof(struct vtvt_data));
    CX=CY=-1;
    DATA->attr=-1;
    DATA->tty=tty;
    
    SX=80; SY=25;
    get_tty_size(fileno(tty), &SX, &SY);
    
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
        fprintf(DATA->tty, "\ec");
        vtvt_dump(vt);
        fflush(stdout);
    }
}
