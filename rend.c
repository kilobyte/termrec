#include <windows.h> // For GetTickCount(), FIXME.
#include "vt100.h"
#include "trend.h"
#include "rend.h"

static int fpal[2][9]=
{
    {
          0xAAAAAA,
        0x000000,
        0xAA0000,
        0x00AA00,
        0xAAAA00,
        0x0000AA,
        0xAA00AA,
        0x00AAAA,
        0xAAAAAA,
    },
    {
#ifdef RV
          0x000000,
#else
          0xFFFFFF,
#endif
        0x555555,
        0xFF5555,
        0x55FF55,
        0xFFFF55,
        0x5555FF,
        0xFF55FF,
        0x55FFFF,
        0xFFFFFF,
    }
};
static int bpal[9]={
#ifdef TL
# ifdef RV
      0x0fFFFFFF,
# else
      0x40000000,
# endif
#else
      0xff000000,
#endif
    0x000000,
    0xAA0000,
    0x00AA00,
    0xAAAA00,
    0x0000AA,
    0xAA00AA,
    0x00AAAA,
    0xAAAAAA,
};

void render_vt(pixel *pic, int psx, int psy, int px, int py, vt100 *vt)
{
    int x,y;
    union {pixel p;int v;} fg,bg,t;
    int attr;
    int bp=(GetTickCount()&0x1ff)-256;
    
    bp=65536-bp*bp;
    bp/=256;
    
//    bp=(bp<0)?256:1;
    
    t.p.r=t.p.g=t.p.b=0x55*bp/256;
    t.p.a=0;
    bp=t.v;
    
    for(y=0;y<vt->sy;y++)
        for(x=0;x<vt->sx;x++)
        {
/*        FIXME
            if ((x+1)*chx>psx)
                fprintf(stderr,"Trying to write past the right border.\n");
            if ((y+1)*chy>psy)
                fprintf(stderr,"Trying to write past the bottom border.\n");
            
*/
            if ((x+1)*chx>psx)
                continue;
            if ((y+1)*chy>psy)
                continue;
            attr=vt->scr[y*vt->sx+x].attr;
            fg.v=fpal[!!(attr&VT100_ATTR_BOLD)][(signed char)(attr)+1];
            bg.v=bpal[(signed char)(attr>>8)+1];
            if (attr&VT100_ATTR_DIM)
            {
                fg.p.r/=2;
                fg.p.g/=2;
                fg.p.b/=2;
            }
            if (attr&VT100_ATTR_BLINK)
            {
#if 0
                bg.p.r=(((unsigned int)bg.p.r)*3+0x80)/4;
                bg.p.g=(((unsigned int)bg.p.g)*3+0x80)/4;
                bg.p.b=(((unsigned int)bg.p.b)*3+0x80)/4;
                bg.p.a=0;
                fg.p.r=(((unsigned int)fg.p.r)*3+0x80)/4;
                fg.p.g=(((unsigned int)fg.p.g)*3+0x80)/4;
                fg.p.b=(((unsigned int)fg.p.b)*3+0x80)/4;
                fg.p.a=0;
#endif
#if 0
                fg.p.r=((int)fg.p.r-bg.p.r)*bp/256+bg.p.r;
                fg.p.g=((int)fg.p.g-bg.p.g)*bp/256+bg.p.g;
                fg.p.b=((int)fg.p.b-bg.p.b)*bp/256+bg.p.b;
                fg.p.a=((int)fg.p.a-bg.p.a)*bp/256+bg.p.a;
#endif
                bg.v+=bp;
                bg.p.a=bg.p.a*(256-(bp&0xff))>>8;
            }
            if (attr&VT100_ATTR_INVERSE)
            {
                t=fg;
                fg=bg;
                bg=t;
            }
            draw_char(pic,vt->scr[y*vt->sx+x].ch,psx,psy,px+x*chx,py+y*chy,
                fg.p,bg.p);
            if (attr&VT100_ATTR_UNDERLINE)
            {
                t.v=0xff000000;
                draw_char(pic,'_',psx,psy,px+x*chx,py+y*chy,
                    fg.p,t.p);
            }
        }
/*
    x=vt->cx;
    y=vt->cy;
    if (x>=vt->sx)
        x=vt->sx-1;
    fg.v=0x80808080;
    bg.v=0xff000000;
    draw_char(pic,'_',psx,psy,px+x*chx,py+y*chy,fg.p,bg.p);
*/
}

void render_pic(pixel *pic, int psx, int psy, int px, int py, pixel *src, int ssx, int ssy)
{
    int x,y;
    int mx,my;
    
    if (px+(mx=ssx)>psx)
        mx=psx-px;
    if (py+(my=ssy)>psy)
        my=psy-py;
    for (y=0;y<my;y++)
        for (x=0;x<mx;x++)
            pic[(py+y)*psx+px+x]=src[y*ssx+x];
}
