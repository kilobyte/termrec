#include <stdlib.h>
#include "trend.h"

grayp *trend_data;
int chx,chy;

void init_trend(int nx, int ny)
{
    free(trend_data);
    chx=nx;
    chy=ny;
    trend_data=malloc(256*chx*chy*sizeof(grayp));
}

#if 0

// 8 bit

void learn_char(pixel *pic, char ch, int psx, int psy, int px, int py)
{
    pixel p;
    unsigned int v;
    int x,y;
    
    for (y=0;y<chy;y++)
        for (x=0;x<chx;x++)
        {
            p=pic[(py+y)*psx+px+x];
            v=((unsigned int)p.r+(unsigned int)p.g+(unsigned int)p.b)/**(unsigned int)p.a/(255*3)*//3;
//            v=(x+y)*255/(chx+chy-2);
            if (!x || !y)
                v=(v*3+0x80)/4;
            trend_data[((unsigned int)(unsigned char)ch)*chx*chy+y*chx+x]=v;
        }
}

void draw_char(pixel *pic, char ch, int psx, int psy, int px, int py, pixel fg, pixel bg)
{
    unsigned int v;
    pixel p,o;
    int x,y;
    
    for (y=0;y<chy;y++)
        for(x=0;x<chx;x++)
        {
            v=trend_data[((unsigned int)(unsigned char)ch)*chx*chy+y*chx+x];
            p.r=blend(fg.r, bg.r, v);
            p.g=blend(fg.g, bg.g, v);
            p.b=blend(fg.b, bg.b, v);
            o=pic[(py+y)*psx+px+x];
            v=p.a;
            p.r=blend(o.r, p.r, v);
            p.g=blend(o.g, p.g, v);
            p.b=blend(o.b, p.b, v);
            pic[(py+y)*psx+px+x]=p;
        }
}
#else

// 24 bit

void learn_char(pixel *pic, char ch, int psx, int psy, int px, int py)
{
    pixel p;
    unsigned int v;
    int x,y;
    
    for (y=0;y<chy;y++)
        for (x=0;x<chx;x++)
        {
            p=pic[(py+y)*psx+px+x];
            v=((unsigned int)p.r+(unsigned int)p.g+(unsigned int)p.b)/**(unsigned int)p.a/(255*3)*//3;
//            if (!x || !y)
//                v=(v*3+0x80)/4;
            p.a=v;
            trend_data[((unsigned int)(unsigned char)ch)*chx*chy+y*chx+x]=p;
        }
}

void draw_char(pixel *pic, char ch, int psx, int psy, int px, int py, pixel fg, pixel bg)
{
    unsigned int v;
    pixel p,o,t;
    int x,y;
    
    for (y=0;y<chy;y++)
        for(x=0;x<chx;x++)
        {
            t=trend_data[((unsigned int)(unsigned char)ch)*chx*chy+y*chx+x];
            p.r=blend(fg.r, bg.r, t.r);
            p.g=blend(fg.g, bg.g, t.g);
            p.b=blend(fg.b, bg.b, t.b);
            p.a=blend(fg.a, bg.a, t.a);
            o=pic[(py+y)*psx+px+x];
            v=p.a;
            
            p.r=blend(o.r, p.r, v);
            p.g=blend(o.g, p.g, v);
            p.b=blend(o.b, p.b, v);
            p.a=(((unsigned int)o.a)*v+255)>>8;
            pic[(py+y)*psx+px+x]=p;
        }
}
#endif

void tile_pic(pixel *pic, int psx, int psy, int px, int py, int sx, int sy, pixel *src, int ssx, int ssy)
{
    int x,y,rx;
    pixel *sr,*sl;

    pic+=py*psx+px;
    // TODO: no division and shit
#if 0
    for (y=0;y<sy;y++)
        for (x=0;x<sx;x++)
            pic[y*psx+x]=src[(y%ssy)*ssx+x%ssx];
#else
    y=ssy;
    sl=src;
    while(sy--)
    {
        if (!(--y))
        {
            y=ssy;
            sl=src;
        }
        rx=sx;
        while(rx>ssx)
        {
            sr=sl;
            x=ssx;
            while(x--)
                *pic++=*sr++;
            rx-=ssx;
        }
        sr=sl;
        while(rx--)
            *pic++=*sr++;
        sl+=ssx;
        pic+=psx-sx;
    }
#endif
}

void draw_pic(pixel *pic, int psx, int psy, int px, int py, pixel *src, int ssx, int ssy)
{
    int x,y;
    int mx,my;
    pixel o,p;
    unsigned int v;
    
    if (px+(mx=ssx)>psx)
        mx=psx-px;
    if (py+(my=ssy)>psy)
        my=psy-py;
    for (y=0;y<my;y++)
        for (x=0;x<mx;x++)
        {
            o=pic[(py+y)*psx+px+x];
            p=src[y*ssx+x];
            v=p.a;
            
            p.r=(o.r*v+p.r*(255-v))/255;
            p.g=(o.g*v+p.g*(255-v))/255;
            p.b=(o.b*v+p.b*(255-v))/255;
            p.a=o.a*v/255;
            pic[(py+y)*psx+px+x]=p;
        }
}

void overlay_pic(pixel *pic, int psx, int psy, int px, int py, pixel *src, int ssx, int ssy, int opacity)
{
    int x,y;
    int mx,my;
    pixel o,p;
    unsigned int v;
    
    if (px+(mx=ssx)>psx)
        mx=psx-px;
    if (py+(my=ssy)>psy)
        my=psy-py;
    for (y=0;y<my;y++)
        for (x=0;x<mx;x++)
        {
            o=pic[(py+y)*psx+px+x];
            p=src[y*ssx+x];
            v=255-((255+(255-p.a)*opacity)>>8);
            
            p.r=blend(o.r, p.r, v);
            p.g=blend(o.g, p.g, v);
            p.b=blend(o.b, p.b, v);
            p.a=o.a*v/255;
            pic[(py+y)*psx+px+x]=p;
        }
}
