//#define VT100_DEBUG
#include "config.h"
#define _GNU_SOURCE
#include "vt100.h"
#include "charsets.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "export.h"
#include "compat.h"

#define SX vt->sx
#define SY vt->sy
#define CX vt->cx
#define CY vt->cy

enum { ESnormal, ESesc, ESgetpars, ESsquare, ESques, ESsetG0, ESsetG1,
       ESpercent, ESosc };

export vt100 vt100_init(int sx, int sy, int resizable, int utf)
{
    vt100 vt;

    vt=malloc(sizeof(struct vt100));
    if (!vt)
        return 0;

    memset(vt, 0, sizeof(struct vt100));
    vt->opt_allow_resize=resizable;
    vt->utf=utf;
    vt100_reset(vt);
    if (sx && sy)
        vt100_resize(vt, sx, sy);
    return vt;
}

export int vt100_resize(vt100 vt, int nsx, int nsy)
{
    int x,y;
    attrchar *nscr;

    if (nsx==SX && nsy==SY)
        return 0;
#ifdef VT100_DEBUG
    printf("Resize from %dx%d to %dx%d\n", SX, SY, nsx, nsy);
#endif
    nscr=malloc(nsx*nsy*sizeof(attrchar));
    for (y=0;y<nsy;y++)
        for (x=0;x<nsx;x++)
        {
            nscr[y*nsx+x].ch=' ';
            nscr[y*nsx+x].attr=vt->attr;
        }
    if (vt->scr)
    {
        for (y=0;y<SY && y<nsy;y++)
            for (x=0;x<SX && x<nsx;x++)
                nscr[y*nsx+x]=vt->scr[y*SX+x];
        free(vt->scr);
    }
    vt->scr=nscr;
    SX=nsx;
    SY=nsy;
    vt->s1=0;
    vt->s2=nsy;
    if (CX>nsx)
        CX=nsx;
    if (CY>=nsy)
        CY=nsy-1;
    if (vt->save_cx>nsx)
        vt->save_cx=nsx;
    if (vt->save_cy>=nsy)
        vt->save_cy=nsy-1;
    return 1;
}

export void vt100_free(vt100 vt)
{
    if (!vt)
        return;

    if (vt->l_free)
        vt->l_free(vt);
    free(vt->scr);
    free(vt);
}

export vt100 vt100_copy(vt100 vt)
{
    vt100 nvt=malloc(sizeof(struct vt100));
    if (!nvt)
        return 0;

    memcpy(nvt, vt, sizeof(struct vt100));
    if (!(nvt->scr=malloc(SX*SY*sizeof(attrchar))))
    {
        free(nvt);
        return 0;
    }
    memcpy(nvt->scr, vt->scr, SX*SY*sizeof(attrchar));
    return nvt;
}

static void vt100_clear_region(vt100 vt, int st, int l)
{
    attrchar *c, blank;

    assert(st>=0);
    assert(l>=0);
    assert(st+l<=SX*SY);
    blank.ch=' ';
    blank.attr=vt->attr;

    c=vt->scr+st;
    while (l--)
        *c++=blank;
}

export void vt100_reset(vt100 vt)
{
    vt100_clear_region(vt, 0, SX*SY);
    CX=CY=vt->save_cx=vt->save_cy=0;
    vt->s1=0;
    vt->s2=SY;
    vt->attr=0x1010;
    vt->state=ESnormal;
    vt->opt_auto_wrap=1;
    vt->opt_cursor=1;
    vt->opt_kpad=0;
    vt->G=1<<1; // G0: normal, G1: vt100 graphics
    vt->curG=0;
    vt->utf_count=0;
}

static void vt100_scroll(vt100 vt, int nl)
{
    int s;

    assert(vt->s1<vt->s2);
    if ((s=vt->s2-vt->s1-abs(nl))<=0)
    {
        vt100_clear_region(vt, vt->s1*SX, (vt->s2-vt->s1)*SX);
        return;
    }
    if (nl<0)
    {
        memmove(vt->scr+(vt->s1-nl)*SX, vt->scr+vt->s1*SX, s*SX*sizeof(attrchar));
        vt100_clear_region(vt, vt->s1*SX, -nl*SX);
    }
    else
    {
        memmove(vt->scr+vt->s1*SX, vt->scr+(vt->s1+nl)*SX, s*SX*sizeof(attrchar));
        vt100_clear_region(vt, (vt->s2-nl)*SX, nl*SX);
    }
}


static void set_charset(vt100 vt, int g, char x)
{
#ifdef VT100_DEBUG
    printf("Changing charset G%d to %c.\n", g, x);
#endif
    switch(x)
    {
        case '0':
            vt->G|=1<<g;
            break;
//      case 'A': UK hash-less -- go away, bad thing.
        case 'B':
        case 'U':
            vt->G&=~(1<<g);
            break;
        default:
#ifdef VT100_DEBUG
        printf("Unknown charset: %c\n", x);
#endif
            return;
    }
}


#define L_CURSOR {if (vt->l_cursor) vt->l_cursor(vt, CX, CY);}
#define FLAG(opt,f,v) \
    {						\
        vt->opt=v;				\
        if (vt->l_flag)				\
            vt->l_flag(vt, f, v);		\
    }
#define SCROLL(nl) \
    {						\
        vt100_scroll(vt, nl);			\
        if (vt->l_scroll)			\
            vt->l_scroll(vt, nl);		\
    }
#define CLEAR(x,y,l) \
    {						\
        vt100_clear_region(vt, y*SX+x, l);	\
        if (vt->l_clear)			\
            vt->l_clear(vt, x, y, l);		\
    }

export void vt100_write(vt100 vt, char *buf, int len)
{
    int i;
    ucs c;

    buf--;
    while (len--)
    {

        switch(*++buf)
        {
        case 0:
            continue;
        case 7:
            if (vt->state == ESosc)
                vt->state = ESnormal;
            continue;
        case 8:
            if (CX)
                CX--;
            L_CURSOR;
            continue;
        case 9:
            if (CX<SX)
                do
                {
                    vt->scr[CY*SX+CX].attr=vt->attr;
                    vt->scr[CY*SX+CX].ch=' ';
                    CX++;
                    if (vt->l_char)
                        vt->l_char(vt, CX-1, CY, ' ', vt->attr);
                } while (CX<SX && CX&7);
            continue;
        case 10:
            CX=0;
        case 11:
        newline:
            CY++;
            if (CY==vt->s2)
            {
                CY=vt->s2-1;
                SCROLL(1);
            }
            else
            {
                if (CY>=SY)
                    CY=SY-1;
                L_CURSOR;
            }
            continue;
        case 12:
            CX=CY=0;
            CLEAR(0, 0, SX*SY);
            L_CURSOR;
            continue;
        case 13:
            CX=0;
            L_CURSOR;
            continue;
        case 14:
            vt->curG=1;
            continue;
        case 15:
            vt->curG=0;
            continue;
        case 24:
        case 26:
            vt->state=ESnormal;
            continue;
        case 27:
            vt->state=ESesc;
            continue;
        case 127:
            continue;
        }

        switch(vt->state)
        {
        error:
#ifdef VT100_DEBUG
            switch(vt->state)
            {
            case ESnormal:
                printf("Unknown code 0x%2x\n", *buf);
                break;
            case ESesc:
                printf("Unknown code ESC %c\n", *buf);
                break;
            case ESsquare: case ESques: case ESpercent:
                printf("Unknown code ESC");
                switch(vt->state)
                {
                case ESsquare:  printf(" ["); break;
                case ESques:    printf(" [ ?"); break;
                case ESpercent: printf(" %%"); break;
                }
                for (i=0;i<=vt->ntok;i++)
                    printf("%c%u%s", i?';':' ', vt->tok[i], vt->tok[i]?"":"?");
                printf(" %c\n", *buf);
                break;
            default:
                printf("Unknown state %d\n", vt->state);
            }
#endif
            vt->state=ESnormal;
            break;

#define ic ((unsigned char)*buf)
#define tc (vt->utf_char)
#define cnt (vt->utf_count)
        case ESnormal:
            if (vt->utf)
                if (ic>0x7f)
                    if (cnt>0 && (ic&0xc0)==0x80)
                    {
                        tc=(tc<<6) | (ic&0x3f);
                        if (!--cnt)
                            c=tc;
                        else
                            continue;

                        // Win32 TextOut (rightfully) chokes on these illegal
                        // chars, so we'd better mark them as invalid.
                        if (c<0xA0)
                            c=0xFFFD;
                        if (c==0xFFEF)	// BOM
                            continue;

                        /* The following code deals with malformed UTF-16
                         * surrogates encoded in UTF-8 text.  While the
                         * standard explicitely forbids this, some (usually
                         * Windows) programs generate them, and thus we'll
                         * better support such encapsulation anyway.
                         * We don't go out of our way to detect unpaired
                         * surrogates, though.
                         */
                        if (c>=0xD800 && c<=0xDFFF)	// UTF-16 surrogates
                        {
                            if (c<0xDC00)	// lead surrogate
                            {
                                vt->utf_surrogate=c;
                                continue;
                            }
                            else		// trailing surrogate
                            {
                                c=(vt->utf_surrogate<<10)+c+
                                    (0x10000 - (0xD800 << 10) - 0xDC00);
                                vt->utf_surrogate=0;
                                if (c<0x10000)	// malformed pair
                                    continue;
                            }
                        }
                    }
                    else
                    {
                        if ((ic&0xe0)==0xc0)
                            cnt=1, tc=ic&0x1f;
                        else if ((ic&0xf0)==0xe0)
                            cnt=2, tc=ic&0x0f;
                        else if ((ic&0xf8)==0xf0)
                            cnt=3, tc=ic&0x07;
                        else if ((ic&0xfc)==0xf8)
                            cnt=4, tc=ic&0x03;
                        else if ((ic&0xfe)==0xfc)
                            cnt=5, tc=ic&0x01;
                        else
                            cnt=0;
                        if (!tc)
                            cnt=0;  // overlong character
                        continue;
                    }
                else
                    cnt=0, c=ic;
            else
                c=charset_cp437[ic];
            if (c>31)
            {
                if (c<128 && vt->G&(1<<vt->curG))
                    c=charset_vt100[c];
                if (CX>=SX)
                {
                    if (vt->opt_auto_wrap)
                    {
                        CX=0;
                        CY++;
                        if (CY>=vt->s2)
                        {
                            CY=vt->s2-1;
                            SCROLL(1);
                        }
                    }
                    else
                        CX=SX-1;
                }
                vt->scr[CY*SX+CX].ch=c;
                vt->scr[CY*SX+CX].attr=vt->attr;
                CX++;
                if (vt->l_char)
                    vt->l_char(vt, CX-1, CY, c, vt->attr);
            }
            break;
#undef ic
#undef tc
#undef cnt

        case ESesc:	// ESC
            switch(*buf)
            {
            case '[':
                vt->state=ESsquare;
                vt->ntok=0;
                vt->tok[0]=0;
                break;

            case ']':
                vt->state=ESosc;
                break;

            case '(':
                vt->state=ESsetG0;
                break;

            case ')':
                vt->state=ESsetG1;
                break;

            case '%':
                vt->state=ESpercent;
                break;

            case '7':		// ESC 7 -> save cursor position
                vt->save_cx=CX;
                vt->save_cy=CY;
                vt->state=ESnormal;
                break;

            case '8':		// ESC 8 -> restore cursor position
                CX=vt->save_cx;
                CY=vt->save_cy;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'D':		// ESC D -> newline
                vt->state=ESnormal;
                goto newline;

            case 'E':		// ESC E -> newline+cr
                vt->state=ESnormal;
                CX=0;
                goto newline;

            case 'M':		// ESC M -> reverse newline
                CY--;
                if (CY==vt->s1-1)
                {
                    CY=vt->s1;
                    SCROLL(-1);
                }
                else
                {
                    if (CY<0)
                        CY=0;
                    L_CURSOR;
                }
                vt->state=ESnormal;
                break;

            case '=':		// ESC = -> application keypad mode
                FLAG(opt_kpad, VT100_FLAG_KPAD, 1);
                vt->state=ESnormal;
                break;

            case '>':		// ESC > -> numeric keypad mode
                FLAG(opt_kpad, VT100_FLAG_KPAD, 0);
                vt->state=ESnormal;
                break;

            default:
                goto error;
            }
            break;

        case ESsquare:	// ESC [, immediately
            if (*buf=='?')
            {
                vt->state=ESques;
                break;
            }
            else
                vt->state=ESgetpars; // fallthru
        case ESgetpars:	// ESC [, with params
            switch(*buf)
            {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                vt->tok[vt->ntok]=vt->tok[vt->ntok]*10+*buf-'0';
                break;

            case ';':
                if (++vt->ntok>=VT100_MAXTOK)
                    goto error;
                vt->tok[vt->ntok]=0;
                break;

            case 'm':	// ESC[m -> change color/attribs
                for (i=0;i<=vt->ntok;i++)
                    switch(vt->tok[i])
                    {
                    case 0:
                        vt->attr=0x1010;
                        break;
                    case 1:
                        vt->attr|=VT100_ATTR_BOLD;
                        vt->attr&=~VT100_ATTR_DIM;
                        break;
                    case 2:
                        vt->attr|=VT100_ATTR_DIM;
                        vt->attr&=~VT100_ATTR_BOLD;
                        break;
                    case 3:
                        vt->attr|=VT100_ATTR_ITALIC;
                        break;
                    case 4:
                        vt->attr|=VT100_ATTR_UNDERLINE;
                        break;
                    case 5:
                        vt->attr|=VT100_ATTR_BLINK;
                        break;
                    case 7:
                        vt->attr|=VT100_ATTR_INVERSE;
                        break;
                    case 21:
                        vt->attr&=~(VT100_ATTR_BOLD|VT100_ATTR_DIM);
                        break;
                    case 22:
                        vt->attr&=~(VT100_ATTR_BOLD|VT100_ATTR_DIM);
                        break;
                    case 23:
                        vt->attr&=~VT100_ATTR_ITALIC;
                        break;
                    case 24:
                        vt->attr&=~VT100_ATTR_UNDERLINE;
                        break;
                    case 25:
                        vt->attr&=~VT100_ATTR_BLINK;
                        break;
                    case 27:
                        vt->attr&=~VT100_ATTR_INVERSE;
                        break;
                    case 30: case 31: case 32: case 33:
                    case 34: case 35: case 36: case 37:
                        vt->attr=(vt->attr&~0xff)|(vt->tok[i]-30);
                        break;
                    case 38:
                        // Other subcommands, none of which we support:
                        // * 2: RGB
                        // * 3: CMY
                        // * 4: CMYK
                        if (vt->tok[++i]!=5)
                            break;
                        if (vt->tok[++i]==16)
                            vt->attr&=~0xff; // colour 16 is same as 0
                        else
                            vt->attr=vt->attr&~0xff|vt->tok[i];
                        break;
                    case 39:
                        vt->attr=vt->attr&~0xff|0x10;
                        break;
                    case 40: case 41: case 42: case 43:
                    case 44: case 45: case 46: case 47:
                        vt->attr=(vt->attr&~0xff00)|(vt->tok[i]-40)<<8;
                        break;
                    case 48:
                        if (vt->tok[++i]!=5)
                            break;
                        if (vt->tok[++i]==16)
                            vt->attr&=~0xff00; // colour 16 is same as 0
                        else
                            vt->attr=vt->attr&~0xff00|vt->tok[i]<<8;
                        break;
                    case 49:
                        vt->attr=vt->attr&~0xff00|0x1000;
                    }
                vt->state=ESnormal;
                break;

            case 'D':	// ESC[D -> move cursor left
                i=vt->tok[0];
                if (i<1)
                    i=1;
                if (CX>i)
                    CX-=i;
                else
                    CX=0;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'C':	case 'a':	// ESC[C -> move cursor right
                i=vt->tok[0];
                if (i<1)
                    i=1;
                if (CX+i<SX)
                    CX+=i;
                else
                    CX=SX;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'A':	// ESC[A -> move cursor up, no scrolling
                i=vt->tok[0];
                if (i<1)
                    i=1;
                if (CY>i)
                    CY-=i;
                else
                    CY=0;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'B':	// ESC[B -> move cursor down, no scrolling
                i=vt->tok[0];
                if (i<1)
                    i=1;
                if (CY+i<SY)
                    CY+=i;
                else
                    CY=SY;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'r':	// ESC[r -> set scrolling region
                if (!vt->tok[0])
                    vt->tok[0]=1;
                if (!vt->ntok)
                    vt->tok[1]=0;
                if (!vt->tok[1])
                    vt->tok[1]=SY;
                if (vt->tok[1]<=SY && vt->tok[0]<vt->tok[1])
                {
                    vt->s1=vt->tok[0]-1;
                    vt->s2=vt->tok[1];
                    CX=0;
                    CY=vt->s1;
                }
                vt->state=ESnormal;
                break;

            case 'J':	// ESC[J -> erase screen
                switch(vt->tok[0])
                {
                case 0: // from cursor
                    CLEAR(CX, CY, SX*SY-(CY*SX+CX));
                    break;
                case 1: // to cursor
                    CLEAR(0, 0, CY*SX+CX);
                    break;
                case 2: // whole screen
                    CLEAR(0, 0, SX*SY);
                }
                vt->state=ESnormal;
                break;

            case 'K':	// ESC[K -> erase line
                switch(vt->tok[0])
                {
                case 0: // from cursor
                    CLEAR(CX, CY, SX-CX);
                    break;
                case 1: // to cursor
                    CLEAR(0, CY, CX);
                    break;
                case 2: // whole line
                    CLEAR(0, CY, SX);
                }
                vt->state=ESnormal;
                break;

            case 'L':	// ESC[L -> insert line
                if (vt->s1>CY || vt->s2<=CY)
                {
                    vt->state=ESnormal;
                    break;
                }
                vt->tok[1]=vt->s1;
                vt->s1=CY;
                i=vt->tok[0];
                if (i<=0)
                    i=1;
                SCROLL(-i);
                vt->s1=vt->tok[1];
                vt->state=ESnormal;
                break;

            case 'M':	// ESC[M -> delete line
                if (vt->s1>CY || vt->s2<=CY)
                {
                    vt->state=ESnormal;
                    break;
                }
                vt->tok[1]=vt->s1;
                vt->s1=CY;
                i=vt->tok[0];
                if (i<=0)
                    i=1;
                SCROLL(i);
                vt->s1=vt->tok[1];
                vt->state=ESnormal;
                break;

            case 'X':	// ESC[X -> erase to the right
                i=vt->tok[0];
                if (i<=0)
                    i=1;
                if (i+CX>SX)
                    i=SX-CX;
                CLEAR(CX, CY, i);
                vt->state=ESnormal;
                break;

            case 'f': case 'H':	// ESC[f, ESC[H -> move cursor
                CY=vt->tok[0]-1;
                if (CY<0)
                    CY=0;
                else if (CY>=SY)
                    CY=SY-1;
                if (!vt->ntok)
                    CX=0;
                else
                    CX=vt->tok[1]-1;
                if (CX<0)
                    CX=0;
                else if (CX>=SX)
                    CX=SX-1;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'G': case '`':	// ESC[G, ESC[` -> move cursor horizontally
                CX=vt->tok[0]-1;
                if (CX<0)
                    CX=0;
                else if (CX>=SX)
                    CX=SX-1;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'd':	// ESC[d -> move cursor vertically
                CY=vt->tok[0]-1;
                if (CY<0)
                    CY=0;
                else if (CY>=SY)
                    CY=SY-1;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'c':	// ESC[c -> reset to power-on defaults
                vt100_reset(vt);
                if (vt->l_clear)
                    vt->l_clear(vt, 0, 0, SX*SY);
                L_CURSOR;
                break;

            case 't':	// ESC[t -> window manipulation
                vt->state=ESnormal;
                switch(vt->tok[0])
                {
                case 8:	    // ESC8;<h>;<w>t -> resize
                    if (!vt->opt_allow_resize)
                        break;
                    while (vt->ntok<2)
                        vt->tok[++vt->ntok]=0;
                    if (vt->tok[1]<=0)
                        vt->tok[1]=SY;
                    if (vt->tok[2]<=0)
                        vt->tok[2]=SX;
                    vt100_resize(vt, vt->tok[2], vt->tok[1]);
                    if (vt->l_resize)
                        vt->l_resize(vt, vt->tok[2], vt->tok[1]);
                    break;
#ifdef VT100_DEBUG
                default:
                    printf("Unknown extended window manipulation: %d\n", vt->tok[0]);
#endif
                }
                break;

            default:
                goto error;
            }
            break;

        case ESques:	// ESC [ ?
            switch(*buf)
            {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                vt->tok[vt->ntok]=vt->tok[vt->ntok]*10+*buf-'0';
                break;

            case ';':
                if (++vt->ntok>=VT100_MAXTOK)
                    goto error;
                vt->tok[vt->ntok]=0;
                break;

            case 'h':
                for (i=0;i<=vt->ntok;i++)
		    switch(vt->tok[i])
		    {
		    case 7:
		        vt->opt_auto_wrap=1;
		        break;
                    case 25:
                        FLAG(opt_cursor, VT100_FLAG_CURSOR, 1);
                        break;
#ifdef VT100_DEBUG
                    default:
                        printf("Unknown option: ?%u\n", vt->tok[i]);
#endif
                    }
                vt->state=ESnormal;
                break;

            case 'l':
                for (i=0;i<=vt->ntok;i++)
		    switch(vt->tok[i])
		    {
		    case 7:
		        vt->opt_auto_wrap=0;
		        break;
                    case 25:
                        FLAG(opt_cursor, VT100_FLAG_CURSOR, 0);
                        break;
#ifdef VT100_DEBUG
                    default:
                        printf("Unknown option: ?%u\n", vt->tok[i]);
#endif
		    }
                vt->state=ESnormal;
                break;

            default:
                goto error;
            }
            break;

        case ESsetG0:	// ESC ( -> set G0 charset
            set_charset(vt, 0, *buf);
            vt->state=ESnormal;
            break;

        case ESsetG1:	// ESC ) -> set G1 charset
            set_charset(vt, 1, *buf);
            vt->state=ESnormal;
            break;

        case ESpercent:	// ESC %
            switch(*buf)
            {
            case '@':	// ESC % @ -> disable UTF-8
                vt->utf=0;
#ifdef VT100_DEBUG
                printf("Disabling UTF-8.\n");
#endif
                break;
            case '8':	// ESC % 8, ESC % G -> enable UTF-8
            case 'G':
                vt->utf=1;
#ifdef VT100_DEBUG
                printf("Enabling UTF-8.\n");
#endif
                break;
            default:
                goto error;
            }
            vt->state=ESnormal;
            break;

        case ESosc:
            break;

        default:
            goto error;
        }
    }

    if (vt->l_flush)
        vt->l_flush(vt);
}

#define BUFFER_SIZE 16384

export void vt100_printf(vt100 vt, const char *fmt, ...)
{
    va_list ap;
    char buf[BUFFER_SIZE], *bigstr;
    int len;

    va_start(ap, fmt);
    len=vsnprintf(buf, BUFFER_SIZE, fmt, ap);
    va_end(ap);
    if (len<=0)
    {
        va_start(ap, fmt);
        len=vasprintf(&bigstr, fmt, ap);
        va_end(ap);
        if (len<=0 || !bigstr)
            return;
        vt100_write(vt, buf, len);
        free(bigstr);
    }
    else if (len>=BUFFER_SIZE)
    {
        if (!(bigstr=malloc(len+1)))
            return;
        va_start(ap, fmt);
        len=vsnprintf(bigstr, len+1, fmt, ap);
        va_end(ap);
        vt100_write(vt, buf, len);
        free(bigstr);
    }
    else
        vt100_write(vt, buf, len);
}
