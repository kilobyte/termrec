#include <windows.h>
#include <stdbool.h>
#include "tty.h"
#include "draw.h"


static HFONT fonts[2][2]; /* underline, strikethrough */
static HBRUSH bg_brush;
int chx, chy;
#define ATTR_ALT_FONT (VT100_ATTR_UNDERLINE|VT100_ATTR_STRIKE)


// win32 has a big endian byte order here!
static int col256_to_rgb(int i, bool bold)
{
    // Standard colours.
    if (i < 16)
    {
        int c = (i&1 ? 0x0000aa : 0x000000)
              | (i&2 ? 0x00aa00 : 0x000000)
              | (i&4 ? 0xaa0000 : 0x000000);
        return (i >= 8 || bold) ? c + 0x555555 : c;
    }
    // 6x6x6 colour cube.
    else if (i < 232)
    {
        i -= 16;
        return (i / 36 * 85 / 2)
             | (i / 6 % 6 * 85 / 2) << 8
             | (i % 6 * 85 / 2) << 16;
    }
    // Grayscale ramp.
    else // i < 256
        return (i * 10 - 2312) * 0x010101;
}


static void draw_line(HDC dc, int x, int y, wchar_t *txt, int cnt, int attr)
{
    union {color p;int v;} fg,bg,t;
    unsigned char c;

    c = attr;
    if (c == 0x10)
        c = 7;
    fg.v=col256_to_rgb(c, attr&VT100_ATTR_BOLD);
    c = attr>>8;
    if (c == 0x10)
        c = 0;
    bg.v=col256_to_rgb(c, false);
    if (attr&VT100_ATTR_DIM)
    {
        fg.p.r/=2;
        fg.p.g/=2;
        fg.p.b/=2;
    }
    if (attr&VT100_ATTR_BLINK)
    {
        bg.p.r=(((unsigned int)bg.p.r)+0x80)/2;
        bg.p.g=(((unsigned int)bg.p.g)+0x80)/2;
        bg.p.b=(((unsigned int)bg.p.b)+0x80)/2;
        fg.p.r=(((unsigned int)fg.p.r)+0x80)/2;
        fg.p.g=(((unsigned int)fg.p.g)+0x80)/2;
        fg.p.b=(((unsigned int)fg.p.b)+0x80)/2;
    }
    if (attr&VT100_ATTR_INVERSE)
    {
        t=fg;
        fg=bg;
        bg=t;
    }
    if (attr&ATTR_ALT_FONT)
        SelectObject(dc, fonts[!!(attr&VT100_ATTR_UNDERLINE)][!!(attr&VT100_ATTR_STRIKE)]);
    SetTextColor(dc, fg.v);
    SetBkColor(dc, bg.v);
    TextOutW(dc, x, y, txt, cnt);
    if (attr&ATTR_ALT_FONT)
        SelectObject(dc, fonts[0][0]);
}


void draw_vt(HDC dc, int px, int py, tty vt)
{
    int x,y,x0;
    int attr;
    wchar_t linebuf[512*2]; // same as the max in tty.c
    int cnt;
    attrchar *ch;
    RECT r;
    HFONT oldfont;

    oldfont=SelectObject(dc, fonts[0][0]);
    ch=vt->scr;
    cnt=0;
    for (y=0;y<vt->sy;y++)
    {
        cnt=0;
        x0=x=0;
        attr=ch->attr;

        while (1)
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
            if (ch->ch>0xffff)  // UTF-16 surrogates
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

void draw_init(LOGFONT *df)
{
    HDC dc;
    LOGFONT lf;
    HFONT oldfont;
    SIZE sf;

    dc=GetDC(0);

    memset(&lf,0,sizeof(LOGFONT));
    lf.lfHeight=df->lfHeight;
    strcpy(lf.lfFaceName, df->lfFaceName);
    lf.lfWeight=df->lfWeight;
    lf.lfItalic=df->lfItalic;
    lf.lfPitchAndFamily=FIXED_PITCH;
    lf.lfQuality=ANTIALIASED_QUALITY;
    lf.lfOutPrecision=OUT_TT_ONLY_PRECIS;
    fonts[0][0]=CreateFontIndirect(&lf);
    oldfont=SelectObject(dc, fonts[0][0]);
    GetTextExtentPoint(dc, "W", 1, &sf);
    chx=sf.cx;
    chy=sf.cy;
    SelectObject(dc, oldfont);
    lf.lfUnderline=1;
    fonts[1][0]=CreateFontIndirect(&lf);
    lf.lfStrikeOut=1;
    lf.lfUnderline=0;
    fonts[0][1]=CreateFontIndirect(&lf);
    lf.lfUnderline=1;
    fonts[1][1]=CreateFontIndirect(&lf);

    ReleaseDC(0, dc);

    bg_brush=GetStockObject(BLACK_BRUSH);
}

void draw_free(void)
{
    DeleteObject(fonts[0][0]);
    DeleteObject(fonts[1][0]);
    DeleteObject(fonts[0][1]);
    DeleteObject(fonts[1][1]);
    DeleteObject(bg_brush);
}

void draw_border(HDC dc, tty vt)
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
