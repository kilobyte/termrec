#include <windows.h>
#include "vt100.h"
#include "draw.h"


/*
The tables below are -1-based -- the indented values are those of the
"default" colour.
*/
static int fpal[2][9]=
{
    {
          0xAAAAAA,
        0x000000,
        0x0000AA,
        0x00AA00,
        0x00AAAA,
        0xAA0000,
        0xAA00AA,
        0xAAAA00,
        0xAAAAAA,
    },
    {
#ifdef RV
          0x000000,
#else
          0xFFFFFF,
#endif
        0x555555,
        0x5555FF,
        0x55FF55,
        0x55FFFF,
        0xFF5555,
        0xFF55FF,
        0xFFFF55,
        0xFFFFFF,
    }
};
static int bpal[9]={
#ifndef TL
# ifdef RV
      0x0fFFFFFF,
# else
      0x40000000,
# endif
#else
      0xff000000,
#endif
    0x000000,
    0x0000AA,
    0x00AA00,
    0x00AAAA,
    0xAA0000,
    0xAA00AA,
    0xAAAA00,
    0xAAAAAA,
};

HFONT norm_font, und_font;
HBRUSH bg_brush;
int chx, chy;


void draw_line(HDC dc, int x, int y, wchar_t *txt, int cnt, int attr)
{
    union {color p;int v;} fg,bg,t;
    
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
        bg.p.r=(((unsigned int)bg.p.r)*3+0x80)/4;
        bg.p.g=(((unsigned int)bg.p.g)*3+0x80)/4;
        bg.p.b=(((unsigned int)bg.p.b)*3+0x80)/4;
        fg.p.r=(((unsigned int)fg.p.r)*3+0x80)/4;
        fg.p.g=(((unsigned int)fg.p.g)*3+0x80)/4;
        fg.p.b=(((unsigned int)fg.p.b)*3+0x80)/4;
    }
    if (attr&VT100_ATTR_INVERSE)
    {
        t=fg;
        fg=bg;
        bg=t;
    }
    if (attr&VT100_ATTR_UNDERLINE)
        SelectObject(dc, und_font);
    SetTextColor(dc, fg.v);
    SetBkColor(dc, bg.v);
    TextOutW(dc, x, y, txt, cnt);
    if (attr&VT100_ATTR_UNDERLINE)
        SelectObject(dc, norm_font);
}


void draw_vt(HDC dc, int px, int py, vt100 *vt)
{
    int x,y,x0;
    int attr;
    wchar_t linebuf[MAX_LINE*2];
    int cnt;
    attrchar *ch;
    RECT r;
    HFONT oldfont;

    oldfont=SelectObject(dc, norm_font);
    ch=vt->scr;
    cnt=0;
    for(y=0;y<vt->sy;y++)
    {
        cnt=0;
        x0=x=0;
        attr=ch->attr;
        
        while(1)
        {
            if (x>=vt->sx || attr!=ch->attr)
            {
                draw_line(dc, x0*chx, y*chy, linebuf, cnt, attr);
                cnt=0;
                x0=x;
                if (x>=vt->sx)
                    break;
                attr=ch->attr;
            }
            if (ch->ch>0xffff)	/* UTF-16 surrogates */
            {
                linebuf[cnt++]=0xD800-(0x10000>>10)+(ch->ch>>10);
                linebuf[cnt++]=0xDC00+(ch->ch&0x3FF);
            }
            else
                linebuf[cnt++]=ch->ch;
            x++;
            ch++;
        }
    }
    x=vt->cx;
    y=vt->cy;
    if (x>=vt->sx)
        x=vt->sx-1;
    if (vt->opt_cursor)
    {
        r.left=x*chx;
        r.top=y*chy+chy-2;
        r.right=x*chx+chx;
        r.bottom=y*chy+chy;
        DrawFocusRect(dc, &r);
    }
/*
    r.left=chx*vt->sx;
    r.top=0;
    r.right=px;
    r.bottom=chy*vt->sy;
    FillRect(dc, &r, bg_brush);
    r.left=0;
    r.top=r.bottom;
    r.bottom=py;
    FillRect(dc, &r, bg_brush);
*/
    SelectObject(dc, oldfont);
}

void draw_init()
{
    HDC dc;
    LOGFONT lf;
    HFONT oldfont;
    SIZE sf;
    
    dc=GetDC(0);
    
    memset(&lf,0,sizeof(LOGFONT));
    lf.lfHeight=16;
    strcpy(lf.lfFaceName, "Courier New");
    lf.lfPitchAndFamily=FIXED_PITCH;
    lf.lfQuality=ANTIALIASED_QUALITY;
    lf.lfOutPrecision=OUT_TT_ONLY_PRECIS;
    norm_font=CreateFontIndirect(&lf);
    oldfont=SelectObject(dc, norm_font);
    GetTextExtentPoint(dc, "W", 1, &sf);
    chx=sf.cx;
    chy=sf.cy;
    SelectObject(dc, oldfont);
    lf.lfUnderline=1;
    und_font=CreateFontIndirect(&lf);
    
    ReleaseDC(0, dc);

    switch(bpal[0]&0xffffff)
    {
    case 0x000000:
        bg_brush=GetStockObject(BLACK_BRUSH);
        break;
    case 0xFFFFFF:
        bg_brush=GetStockObject(WHITE_BRUSH);
        break;
    default:
        bg_brush=CreateSolidBrush(bpal[0]&0xffffff);
    }
}

void draw_border(HDC dc, vt100 *vt)
{
    int sx,sy;
    HPEN pen, oldpen;
    
    sx=vt->sx*chx;
    sy=vt->sy*chy;

    pen=CreatePen(PS_SOLID, 0, clWoodenDown);
    oldpen=SelectObject(dc, pen);
    MoveToEx(dc, sx, 0, 0);
    LineTo(dc, sx, sy);
    LineTo(dc, -1, sy);
    pen=CreatePen(PS_SOLID, 0, clWoodenMid);
    DeleteObject(SelectObject(dc, pen));
    MoveToEx(dc, sx+1, 0, 0);
    LineTo(dc, sx+1, sy+1);
    LineTo(dc, -1, sy+1);
    DeleteObject(SelectObject(dc, oldpen));
}
