//#define VT100_DEBUG
#include "config.h"
#define _GNU_SOURCE
#include "tty.h"
#include "charsets.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "export.h"
#include "compat.h"
#include "wcwidth.h"

#if defined __GNUC__ && (__GNUC__ >= 7)
 #define FALLTHRU __attribute__((fallthrough))
#else
 #define FALLTHRU
#endif

#ifndef VT100_DEBUG
 #define debuglog(...) {}
#endif

#define SX vt->sx
#define SY vt->sy
#define CX vt->cx
#define CY vt->cy
#define FREECOMB vt->combs[0].next
#define NCOMBS   vt->combs[0].ch

enum { ESnormal, ESesc, ESgetpars, ESsquare, ESques, ESsetG0, ESsetG1,
       ESpercent, ESosc };

static void tty_clear_comb(tty vt, attrchar *ac)
{
    // Must have been initialized before.
    if (!ac->comb)
        return;

    uint32_t i=ac->comb;
    ac->comb=0;

    while (i)
    {
        assert(i < NCOMBS);
        uint32_t j = vt->combs[i].next;
        vt->combs[i].next = FREECOMB;
        FREECOMB = i;
        i=j;
    }
}

static void tty_add_comb(tty vt, attrchar *ac, ucs ch)
{
    if (!vt->combs || FREECOMB >= NCOMBS)
    {
        uint32_t oldn = vt->combs? NCOMBS : 0;
        uint32_t newn = oldn? oldn*2 : 32;
        debuglog("Reallocating combs from %u to %u\n", oldn, newn);
        combc *newcombs = realloc(vt->combs, newn*sizeof(combc));
        if (!newcombs)
            return;
        for (uint32_t i=oldn; i<newn; i++)
            newcombs[i].next=i+1;
        vt->combs=newcombs;
        NCOMBS=newn;
    }

    int clen=0;
    uint32_t *cc=&ac->comb;
    while (*cc)
    {
        assert(*cc<NCOMBS);
        if (++clen>=4)
            return; // ignore if too many marks in one cell
        cc=&vt->combs[*cc].next;
    }

    uint32_t new=*cc=FREECOMB;
    FREECOMB=vt->combs[new].next;
    vt->combs[new].next=0;
    vt->combs[new].ch=ch;

    if (vt->l_comb)
        vt->l_comb(vt, (ac-vt->scr)%SX, (ac-vt->scr)/SX, ch, ac->attr);
}

export tty tty_init(int sx, int sy, int resizable)
{
    tty vt;

    vt=malloc(sizeof(struct tty));
    if (!vt)
        return 0;

    memset(vt, 0, sizeof(struct tty));
    vt->allow_resize=resizable;
    tty_reset(vt);
    if (sx && sy)
        tty_resize(vt, sx, sy);
    return vt;
}

export int tty_resize(tty vt, int nsx, int nsy)
{
    int x,y;
    attrchar *nscr;

    if (nsx==SX && nsy==SY)
        return 0;
    debuglog("Resize from %dx%d to %dx%d\n", SX, SY, nsx, nsy);
    nscr=malloc(nsx*nsy*sizeof(attrchar));
    if (!nscr)
        return 0;
    for (y=0;y<nsy;y++)
        for (x=0;x<nsx;x++)
        {
            nscr[y*nsx+x].ch=' ';
            nscr[y*nsx+x].comb=0;
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

export void tty_free(tty vt)
{
    if (!vt)
        return;

    if (vt->l_free)
        vt->l_free(vt);
    free(vt->scr);
    free(vt->combs);
    free(vt);
}

export tty tty_copy(tty vt)
{
    tty nvt=malloc(sizeof(struct tty));
    if (!nvt)
        return 0;
    memcpy(nvt, vt, sizeof(struct tty));
    if (!(nvt->scr=malloc(SX*SY*sizeof(attrchar))))
        goto drop_nvt;
    memcpy(nvt->scr, vt->scr, SX*SY*sizeof(attrchar));
    if (vt->combs)
    {
        if (!(nvt->combs=malloc(NCOMBS*sizeof(combc))))
            goto drop_scr;
        memcpy(nvt->combs, vt->combs, NCOMBS*sizeof(combc));
    }
    return nvt;

drop_scr:
    free(nvt->scr);
drop_nvt:
    free(nvt);
    return 0;
}

static void tty_clear_region(tty vt, int st, int l)
{
    attrchar *c, blank;

    assert(st>=0);
    assert(l>=0);
    assert(st+l<=SX*SY);
    blank.ch=' ';
    blank.comb=0;
    blank.attr=vt->attr;

    c=vt->scr+st;
    while (l--)
    {
        tty_clear_comb(vt, c);
        *c++=blank;
    }
}

export void tty_reset(tty vt)
{
    tty_clear_region(vt, 0, SX*SY);
    CX=CY=vt->save_cx=vt->save_cy=0;
    vt->s1=0;
    vt->s2=SY;
    vt->attr=0x1010;
    vt->state=ESnormal;
    vt->opt_auto_wrap=1;
    vt->opt_cursor=1;
    vt->opt_kpad=0;
    vt->G=vt->save_G=1<<1; // G0: normal, G1: vt100 graphics
    vt->curG=vt->save_curG=0;
    vt->utf_count=0;
}

static void tty_scroll(tty vt, int nl)
{
    int s;

    assert(vt->s1<vt->s2);
    if ((s=vt->s2-vt->s1-abs(nl))<=0)
    {
        tty_clear_region(vt, vt->s1*SX, (vt->s2-vt->s1)*SX);
        return;
    }
    if (nl<0)
    {
        memmove(vt->scr+(vt->s1-nl)*SX, vt->scr+vt->s1*SX, s*SX*sizeof(attrchar));
        tty_clear_region(vt, vt->s1*SX, -nl*SX);
    }
    else
    {
        memmove(vt->scr+vt->s1*SX, vt->scr+(vt->s1+nl)*SX, s*SX*sizeof(attrchar));
        tty_clear_region(vt, (vt->s2-nl)*SX, nl*SX);
    }
}


static void set_charset(tty vt, int g, char x)
{
    debuglog("Changing charset G%d to %c.\n", g, x);
    switch (x)
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
        debuglog("Unknown charset: %c\n", x);
            return;
    }
}

static void cjk_damage_left(tty vt, int x, bool notify)
{
    assert(x>=0);
    assert(x<SX);
    attrchar *ca = &vt->scr[CY*SX+x];
    if (!(ca->attr & VT100_ATTR_CJK))
        return;
    if (ca->ch != VT100_CJK_RIGHT)
        return;
    if (!x)
        return; // shouldn't happen

    --ca; --x;
    ca->ch=' ';
    ca->attr&=~VT100_ATTR_CJK;
    tty_clear_comb(vt, ca);
    if (notify && vt->l_char)
        vt->l_char(vt, x, CY, ' ', ca->attr, 1);
}

static void cjk_damage_right(tty vt, int x, bool notify)
{
    assert(x>=0);
    assert(x<SX);
    attrchar *ca = &vt->scr[CY*SX+x];
    if (!(ca->attr & VT100_ATTR_CJK))
        return;
    if (mk_wcwidth(ca->ch) != 2 || x>=SX)
        return; // shouldn't happen

    ++ca; ++x;
    ca->ch=' ';
    ca->attr&=~VT100_ATTR_CJK;
    tty_clear_comb(vt, ca);
    if (notify && vt->l_char)
        vt->l_char(vt, x, CY, ' ', ca->attr, 1);
}


#define L_CURSOR {if (vt->l_cursor) vt->l_cursor(vt, CX, CY);}
#define FLAG(opt,f,v) \
    {                                           \
        vt->opt=v;                              \
        if (vt->l_flag)                         \
            vt->l_flag(vt, f, v);               \
    }
#define SCROLL(nl) \
    {                                           \
        tty_scroll(vt, nl);                     \
        if (vt->l_scroll)                       \
            vt->l_scroll(vt, nl);               \
    }
#define CLEAR(x,y,l) \
    {                                           \
        tty_clear_region(vt, y*SX+x, l);        \
        if (vt->l_clear)                        \
            vt->l_clear(vt, x, y, l);           \
    }

static inline void tty_write_char(tty vt, ucs c)
{
    int w=mk_wcwidth(c);

    if (w<0)
        return;
    else if (!w)
    {
        if (!CX)
            return; // combining are illegal at left edge of the screen
        return tty_add_comb(vt, &vt->scr[CY*SX+CX-1], c);
    }

    if (c<128 && vt->G&(1<<vt->curG))
        c=charset_vt100[c];
    if (CX+w>SX)
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
            CX=SX-w;
    }

    // check for damage to existent CJK characters
    // * overwriting half-aligned trailing half
    cjk_damage_left(vt, CX, true);
    // * overwriting 2-width with 1-width, or half-aligned with 2-width
    cjk_damage_right(vt, CX+w-1, false);

    vt->scr[CY*SX+CX].ch=c;
    vt->scr[CY*SX+CX].attr=vt->attr | (w==2?VT100_ATTR_CJK:0);
    tty_clear_comb(vt, &vt->scr[CY*SX+CX]);
    if (w==2)
    {
        vt->scr[CY*SX+CX+1].ch=VT100_CJK_RIGHT;
        vt->scr[CY*SX+CX+1].attr=vt->attr|VT100_ATTR_CJK;
        tty_clear_comb(vt, &vt->scr[CY*SX+CX+1]);
    }
    CX+=w;
    if (vt->l_char)
        vt->l_char(vt, CX-1, CY, c, vt->attr, w);
}

export void tty_write(tty vt, const char *buf, int len)
{
    int i;
    ucs c;

    buf--;
    while (len--)
    {

        switch (*++buf)
        {
        case 0:
        case 5: // ENQ -> ask for terminal's name
            continue;
        case 7: // BEL -> beep; also terminates OSC
            if (vt->state == ESosc)
                vt->state = ESnormal;
            else if (vt->l_bell)
                vt->l_bell(vt);
            continue;
        case 8: // backspace
            if (CX)
                CX--;
            L_CURSOR;
            continue;
        case 9: // tab
            if (CX<SX)
                do
                {
                    vt->scr[CY*SX+CX].attr=vt->attr;
                    vt->scr[CY*SX+CX].ch=' ';
                    tty_clear_comb(vt, &vt->scr[CY*SX+CX]);
                    CX++;
                    if (vt->l_char)
                        vt->l_char(vt, CX-1, CY, ' ', vt->attr, 1);
                } while (CX<SX && CX&7);
            continue;
        case 10: // LF (newline)
            CX=0;
            FALLTHRU;
        case 11: // VT (vertical tab)
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
        case 12: // Form Feed
            CX=CY=0;
            CLEAR(0, 0, SX*SY);
            L_CURSOR;
            continue;
        case 13: // CR
            CX=0;
            L_CURSOR;
            continue;
        case 14: // S0 -> select character set
            vt->curG=1;
            continue;
        case 15: // S1 -> select character set
            vt->curG=0;
            continue;
        case 24: // ???
        case 26: // ???
            vt->state=ESnormal;
            continue;
        case 27: // ESC
            vt->state=ESesc;
            continue;
        case 127:
            continue;
        }

        switch (vt->state)
        {
        error:
#ifdef VT100_DEBUG
            switch (vt->state)
            {
            case ESnormal:
                debuglog("Unknown code 0x%2x\n", *buf);
                break;
            case ESesc:
                debuglog("Unknown code ESC %c\n", *buf);
                break;
            case ESsquare: case ESgetpars: case ESques: case ESpercent:
                debuglog("Unknown code ESC");
                switch (vt->state)
                {
                case ESgetpars:
                case ESsquare:  debuglog(" ["); break;
                case ESques:    debuglog(" [ ?"); break;
                case ESpercent: debuglog(" %%"); break;
                }
                for (i=0;i<=vt->ntok;i++)
                    debuglog("%c%u%s", i?';':' ', vt->tok[i], vt->tok[i]?"":"?");
                debuglog(" %c\n", *buf);
                break;
            default:
                debuglog("Unknown state %d\n", vt->state);
            }
#endif
            vt->state=ESnormal;
            break;

#define ic ((unsigned char)*buf)
#define tc (vt->utf_char)
#define cnt (vt->utf_count)
        case ESnormal:
            if (vt->cp437)
                c=charset_cp437[ic];
            else if (ic<0x80)
                cnt=0, c=ic;
            else if (cnt>0 && (ic&0xc0)==0x80)
            {
                tc=(tc<<6) | (ic&0x3f);
                if (!--cnt)
                    c=tc;
                else
                    continue;

                if (c<0xA0) // underlong or C1 controls
                    c=0xFFFD;
                if (c==0xFFEF)  // BOM
                    continue;

                /* The following code deals with malformed UTF-16
                 * surrogates encoded in UTF-8 text.  While the
                 * standard explicitely forbids this, some (usually
                 * Windows) programs generate them, and thus we'll
                 * better support such encapsulation anyway.
                 * We don't go out of our way to detect unpaired
                 * surrogates, though.
                 */
                if (c>=0xD800 && c<=0xDFFF)     // UTF-16 surrogates
                {
                    if (c<0xDC00)                   // lead surrogate
                    {
                        vt->utf_surrogate=c;
                        continue;
                    }
                    else if (vt->utf_surrogate)     // trailing surrogate
                    {
                        c=(vt->utf_surrogate<<10)+c+
                            (0x10000 - (0xD800 << 10) - 0xDC00);
                        vt->utf_surrogate=0;
                    }
                    else
                        c=0xFFFD;                   // unpaired trailing
                }

                if (c>0x10FFFF) // outside Unicode
                    c=0xFFFD;
                else if ((c&0xFFFF)>=0xFFFE || c>=0xFDD0 && c<0xFDF0)
                    c=0xFFFD; // non-characters
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
                continue;
            }
            tty_write_char(vt, c);
            break;
#undef ic
#undef tc
#undef cnt

        case ESesc:     // ESC
            switch (*buf)
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

            case '7':           // ESC 7 -> save cursor position
                vt->save_cx=CX;
                vt->save_cy=CY;
                vt->save_attr=vt->attr;
                vt->save_G=vt->G;
                vt->save_curG=vt->curG;
                vt->state=ESnormal;
                break;

            case '8':           // ESC 8 -> restore cursor position
                CX=vt->save_cx;
                CY=vt->save_cy;
                vt->attr=vt->save_attr;
                vt->G=vt->save_G;
                vt->curG=vt->save_curG;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'D':           // ESC D -> newline
                vt->state=ESnormal;
                goto newline;

            case 'E':           // ESC E -> newline+cr
                vt->state=ESnormal;
                CX=0;
                goto newline;

            case 'M':           // ESC M -> reverse newline
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

            case '=':           // ESC = -> application keypad mode
                FLAG(opt_kpad, VT100_FLAG_KPAD, 1);
                vt->state=ESnormal;
                break;

            case '>':           // ESC > -> numeric keypad mode
                FLAG(opt_kpad, VT100_FLAG_KPAD, 0);
                vt->state=ESnormal;
                break;

            default:
                goto error;
            }
            break;

        case ESsquare:  // ESC [, immediately
            if (*buf=='?')
            {
                vt->state=ESques;
                break;
            }
            else
                vt->state=ESgetpars; // fallthru
        case ESgetpars: // ESC [, with params
            switch (*buf)
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

            case 'm':   // ESC[m -> change color/attribs
                for (i=0;i<=vt->ntok;i++)
                    switch (vt->tok[i])
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
                    case 9:
                        vt->attr|=VT100_ATTR_STRIKE;
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
                    case 29:
                        vt->attr&=~VT100_ATTR_STRIKE;
                        break;
                    case 30: case 31: case 32: case 33:
                    case 34: case 35: case 36: case 37:
                        vt->attr=(vt->attr&~0xff)|(vt->tok[i]-30);
                        break;
                    case 38:
                        if (i>=VT100_MAXTOK-1)
                            break;
                        switch (vt->tok[++i])
                        {
                        case 5:
                            if (i>=VT100_MAXTOK-1)
                                break;
                            if (vt->tok[++i]==16)
                                vt->attr&=~0xff; // colour 16 is same as 0
                            else
                                vt->attr=vt->attr&~0xff|vt->tok[i];
                            break;
                        case 2:
                        case 3:
                            i+=3;
                            break;
                        case 4:
                            i+=4;
                        }
                        break;
                    case 39:
                        vt->attr=vt->attr&~0xff|0x10;
                        break;
                    case 40: case 41: case 42: case 43:
                    case 44: case 45: case 46: case 47:
                        vt->attr=(vt->attr&~0xff00)|(vt->tok[i]-40)<<8;
                        break;
                    case 48:
                        if (i>=VT100_MAXTOK-1)
                            break;
                        switch (vt->tok[++i])
                        {
                        case 5:
                            if (i>=VT100_MAXTOK-1)
                                break;
                            if (vt->tok[++i]==16)
                                vt->attr&=~0xff00; // colour 16 is same as 0
                            else
                                vt->attr=vt->attr&~0xff00|vt->tok[i]<<8;
                            break;
                        case 2:
                        case 3:
                            i+=3;
                            break;
                        case 4:
                            i+=4;
                        }
                        break;
                    case 49:
                        vt->attr=vt->attr&~0xff00|0x1000;
                        break;
                    case 90: case 91: case 92: case 93:
                    case 94: case 95: case 96: case 97:
                        vt->attr=(vt->attr&~0xff)|(vt->tok[i]-90+8);
                        break;
                    case 100: case 101: case 102: case 103:
                    case 104: case 105: case 106: case 107:
                        vt->attr=(vt->attr&~0xff00)|(vt->tok[i]-100+8)<<8;
                        break;
                    }
                vt->state=ESnormal;
                break;

            case 'D':   // ESC[D -> move cursor left
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

            case 'C':   case 'a':       // ESC[C -> move cursor right
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

            case 'F':   // ESC[F -> move cursor up and home, no scrolling
                CX=0;
                FALLTHRU;
            case 'A':   // ESC[A -> move cursor up, no scrolling
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

            case 'E':   // ESC[E -> move cursor down and home, no scrolling
                CX=0;
                FALLTHRU;
            case 'B':   // ESC[B -> move cursor down, no scrolling
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

            case 'r':   // ESC[r -> set scrolling region
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

            case 'J':   // ESC[J -> erase screen
                switch (vt->tok[0])
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

            case 'K':   // ESC[K -> erase line
                switch (vt->tok[0])
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

            case 'L':   // ESC[L -> insert line
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

            case 'M':   // ESC[M -> delete line
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

            case '@':   // ESC[@ -> insert spaces
                i=vt->tok[0];
                if (i<=0)
                    i=1;
                if (i+CX>SX)
                    i=SX-CX;
                if (i<=0)
                    break;
                for (int j=SX-CX-i-1;j>=0;j--)
                    vt->scr[CY*SX+CX+j+i]=vt->scr[CY*SX+CX+j];
                tty_clear_region(vt, CY*SX+CX, i);
                if (vt->l_char)
                    for (int j=CX;j<SX;j++)
                        vt->l_char(vt, j, CY, vt->scr[CY*SX+j].ch,
                                              vt->scr[CY*SX+j].attr,
                                              mk_wcwidth(vt->scr[CY*SX+j].ch));
                vt->state=ESnormal;
                break;

            case 'P':   // ESC[P -> delete characters
                i=vt->tok[0];
                if (i<=0)
                    i=1;
                if (i+CX>SX)
                    i=SX-CX;
                if (i<=0)
                    break;
                for (int j=CX+i;j<SX;j++)
                {
                    vt->scr[CY*SX+j-i]=vt->scr[CY*SX+j];
                    if (vt->l_char)
                    {
                        vt->l_char(vt, j-i, CY, vt->scr[CY*SX+j].ch,
                                                vt->scr[CY*SX+j].attr,
                                                mk_wcwidth(vt->scr[CY*SX+j].ch));
                    }
                }
                CLEAR(SX-i, CY, i);
                vt->state=ESnormal;
                break;

            case 'X':   // ESC[X -> erase to the right
                i=vt->tok[0];
                if (i<=0)
                    i=1;
                if (i+CX>SX)
                    i=SX-CX;
                CLEAR(CX, CY, i);
                vt->state=ESnormal;
                break;

            case 'f': case 'H': // ESC[f, ESC[H -> move cursor
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

            case 'G': case '`': // ESC[G, ESC[` -> move cursor horizontally
                CX=vt->tok[0]-1;
                if (CX<0)
                    CX=0;
                else if (CX>=SX)
                    CX=SX-1;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'd':   // ESC[d -> move cursor vertically
                CY=vt->tok[0]-1;
                if (CY<0)
                    CY=0;
                else if (CY>=SY)
                    CY=SY-1;
                L_CURSOR;
                vt->state=ESnormal;
                break;

            case 'c':   // ESC[c -> reset to power-on defaults
                tty_reset(vt);
                if (vt->l_clear)
                    vt->l_clear(vt, 0, 0, SX*SY);
                L_CURSOR;
                break;

            case 't':   // ESC[t -> window manipulation
                vt->state=ESnormal;
                switch (vt->tok[0])
                {
                case 8:     // ESC[8;<h>;<w>t -> resize
                    if (!vt->allow_resize)
                        break;
                    while (vt->ntok<2)
                        vt->tok[++vt->ntok]=0;
                    if (vt->tok[1]>256 || vt->tok[2]>512) // arbitrary
                        break; // sanity check
                    if (vt->tok[1]<=0)
                        vt->tok[1]=SY;
                    if (vt->tok[2]<=0)
                        vt->tok[2]=SX;
                    tty_resize(vt, vt->tok[2], vt->tok[1]);
                    if (vt->l_resize)
                        vt->l_resize(vt, vt->tok[2], vt->tok[1]);
                    break;
                default:
                    debuglog("Unknown extended window manipulation: %d\n", vt->tok[0]);
                }
                break;

            default:
                goto error;
            }
            break;

        case ESques:    // ESC [ ?
            switch (*buf)
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
                    switch (vt->tok[i])
                    {
                    case 7:
                        vt->opt_auto_wrap=1;
                        break;
                    case 25:
                        FLAG(opt_cursor, VT100_FLAG_CURSOR, 1);
                        break;
                    default:
                        debuglog("Unknown option: ?%u\n", vt->tok[i]);
                    }
                vt->state=ESnormal;
                break;

            case 'l':
                for (i=0;i<=vt->ntok;i++)
                    switch (vt->tok[i])
                    {
                    case 7:
                        vt->opt_auto_wrap=0;
                        break;
                    case 25:
                        FLAG(opt_cursor, VT100_FLAG_CURSOR, 0);
                        break;
                    default:
                        debuglog("Unknown option: ?%u\n", vt->tok[i]);
                    }
                vt->state=ESnormal;
                break;

            default:
                goto error;
            }
            break;

        case ESsetG0:   // ESC ( -> set G0 charset
            set_charset(vt, 0, *buf);
            vt->state=ESnormal;
            break;

        case ESsetG1:   // ESC ) -> set G1 charset
            set_charset(vt, 1, *buf);
            vt->state=ESnormal;
            break;

        case ESpercent: // ESC %
            switch (*buf)
            {
            case '@':   // ESC % @ -> disable UTF-8
                vt->cp437=1;
                debuglog("Disabling UTF-8.\n");
                break;
            case '8':   // ESC % 8, ESC % G -> enable UTF-8
            case 'G':
                vt->cp437=0;
                debuglog("Enabling UTF-8.\n");
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

export void tty_printf(tty vt, const char *fmt, ...)
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
        tty_write(vt, buf, len);
        free(bigstr);
    }
    else if (len>=BUFFER_SIZE)
    {
        if (!(bigstr=malloc(len+1)))
            return;
        va_start(ap, fmt);
        len=vsnprintf(bigstr, len+1, fmt, ap);
        va_end(ap);
        tty_write(vt, buf, len);
        free(bigstr);
    }
    else
        tty_write(vt, buf, len);
}
