/*#define VT100_DEBUG*/
#define VT100_DEFAULT_CHARSET charset_cp437

#include "vt100.h"
#include "charsets.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "export.h"

#define SX vt->sx
#define SY vt->sy
#define CX vt->cx
#define CY vt->cy

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
    for(y=0;y<nsy;y++)
        for(x=0;x<nsx;x++)
        {
            nscr[y*nsx+x].ch=' ';
            nscr[y*nsx+x].attr=vt->attr;
        }
    if (vt->scr)
    {
        for(y=0;y<SY && y<nsy;y++)
            for(x=0;x<SX && x<nsx;x++)
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
    
    blank.ch=' ';
    blank.attr=vt->attr;

    c=vt->scr+st;
    while(l--)
        *c++=blank;
}

export void vt100_reset(vt100 vt)
{
    vt100_clear_region(vt, 0, SX*SY);
    CX=CY=vt->save_cx=vt->save_cy=0;
    vt->s1=0;
    vt->s2=SY;
    vt->attr=0xffff;
    vt->state=0;
    vt->opt_auto_wrap=1;
    vt->opt_cursor=1;
    vt->opt_kpad=0;
    vt->G[0]=VT100_DEFAULT_CHARSET;
    vt->G[1]=charset_vt100;
    vt->transl=VT100_DEFAULT_CHARSET;
    vt->curG=0;
    vt->utf=0;
    vt->utf_count=0;
}

static void vt100_scroll(vt100 vt, int nl)
{
    int s;

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
    vt100_charset cs;
    
#ifdef VT100_DEBUG
    printf("Changing charset G%d to %c.\n", g, x);
#endif
    switch(x)
    {
        case '0':
            cs=charset_vt100;
            break;
/*
        case 'A': UK hash-less -- go away, bad thing.
*/
        case 'B':
            cs=/*charset_8859_1*/VT100_DEFAULT_CHARSET;
            break;
        case 'U':
            cs=charset_cp437;
            break;
        default:
#ifdef VT100_DEBUG
        printf("Unknown charset: %c\n", x);
#endif
            return;
    }

    vt->G[g]=cs;
    if (vt->curG==g)
        vt->transl=cs;
}


#define L_CURSOR {if (vt->l_cursor) vt->l_cursor(vt, CX, CY);}
#define FLAG(f,v) \
    {						\
        vt->flags[f]=v;				\
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
        vt100_clear_region(vt, y*SX+x, len);	\
        if (vt->l_clear)			\
            vt->l_clear(vt, x, y, len);		\
    }

export void vt100_write(vt100 vt, char *buf, int len)
{
    int i;
    ucs c;

    buf--;
    while(len--)
    {

        switch(*++buf)
        {
        case 0:
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
                } while(CX<SX && CX&7);
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
            vt->transl=vt->G[1];
            continue;
        case 15:
            vt->curG=0;
            vt->transl=vt->G[0];
            continue;
        case 24:
        case 26:
            vt->state=0;
            continue;
        case 27:
            vt->state=1;
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
            case 0:
                printf("Unknown code 0x%2x\n", *buf);
                break;
            case 1:
                printf("Unknown code ESC %c\n", *buf);
                break;
            case 3: case 4: case 7:
                printf("Unknown code ESC");
                switch(vt->state)
                {
                case 3: printf(" ["); break;
                case 4: printf(" [ ?"); break;
                case 7: printf(" %%"); break;
                }
                for(i=0;i<=vt->ntok;i++)
                    printf("%c%u%s", i?';':' ', vt->tok[i], vt->tok[i]?"":"?");
                printf(" %c\n", *buf);
                break;
            default:
                printf("Unknown state %d\n", vt->state);
            }
#endif
            vt->state=0;
            break;

#define ic ((unsigned char)*buf)
#define tc (vt->utf_char)
#define cnt (vt->utf_count)
        case 0:
            if (vt->utf)
                if (ic>0x7f)
                    if (cnt>0 && (ic&0xc0)==0x80)
                    {
                        tc=(tc<<6) | (ic&0x3f);
                        if (!--cnt)
                            c=tc;
                        else
                            continue;
                        
                        /* Win32 TextOut (rightfully) chokes on these illegal
                         * chars, so we'd better mark them as invalid.
                         */
                        if (c<0xA0)
                            c=0xFFFD;
                        if (c==0xFFEF)	/* BOM */
                            continue;
                            
                        /* The following code deals with malformed UTF-16
                         * surrogates encoded in UTF-8 text.  While the
                         * standard explicitely forbids this, some (usually
                         * Windows) programs generate them, and thus we'll
                         * better support such encapsulation anyway.
                         * We don't go out of our way to detect unpaired
                         * surrogates, though.
                         */
                        if (c>=0xD800 && c<=0xDFFF)	/* UTF-16 surrogates */
                        {
                            if (c<0xDC00)	/* lead surrogate */
                            {
                                vt->utf_surrogate=c;
                                continue;
                            }
                            else		/* trailing surrogate */
                            {
                                c=(vt->utf_surrogate<<10)+c+
                                    (0x10000 - (0xD800 << 10) - 0xDC00);
                                vt->utf_surrogate=0;
                                if (c<0x10000)	/* malformed pair */
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
                            cnt=0;  /* overlong character */
                        continue;
                    }
                else
                    cnt=0, c=ic;
            else
                c=vt->transl[ic];
            if (c>31)
            {
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
            
        case 1:		/* ESC */
            switch(*buf)
            {
            case '[':
                vt->state=3;
                vt->ntok=0;
                vt->tok[0]=0;
                break;
            
            case '(':
                vt->state=5;
                break;
            
            case ')':
                vt->state=6;
                break;
            
            case '%':
                vt->state=7;
                break;
                
            case '7':		/* ESC 7 -> save cursor position */
                vt->save_cx=CX;
                vt->save_cy=CY;
                vt->state=0;
                break;
                
            case '8':		/* ESC 8 -> restore cursor position */
                CX=vt->save_cx;
                CY=vt->save_cy;
                L_CURSOR;
                vt->state=0;
                break;
                
            case 'D':		/* ESC D -> newline */
                vt->state=0;
                goto newline;
                
            case 'E':		/* ESC E -> newline+cr */
                vt->state=0;
                CX=0;
                goto newline;
                
            case 'M':		/* ESC M -> reverse newline */
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
                vt->state=0;
                break;
            
            case '=':		/* ESC = -> application keypad mode */
                FLAG(VT100_FLAG_KPAD, 1);
                vt->state=0;
                break;
            
            case '>':		/* ESC > -> numeric keypad mode */
                FLAG(VT100_FLAG_KPAD, 0);
                vt->state=0;
                break;
                
            default:
                goto error;
            }
            break;
        
        case 3:		/* ESC [, immediately */
            if (*buf=='?')
            {
                vt->state=4;
                break;
            }	/* fallthru */
        case 2:		/* ESC [, with params */
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
                
            case 'm':	/* ESC[m -> change color/attribs */
                for(i=0;i<=vt->ntok;i++)
                    switch(vt->tok[i])
                    {
                    case 0:
                        vt->attr=0xffff;
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
                    case 39:
                        vt->attr|=0xff;
                        break;
                    case 40: case 41: case 42: case 43:
                    case 44: case 45: case 46: case 47:
                        vt->attr=(vt->attr&~0xff00)|(vt->tok[i]-40)<<8;
                        break;
                    case 49:
                        vt->attr|=0xff00;
                    }
                vt->state=0;
                break;
                                
            case 'D':	/* ESC[D -> move cursor left */
                i=vt->tok[0];
                if (i<1)
                    i=1;
                if (CX>i)
                    CX-=i;
                else
                    CX=0;
                L_CURSOR;
                vt->state=0;
                break;
                
            case 'C':	case 'a':	/* ESC[C -> move cursor right */
                i=vt->tok[0];
                if (i<1)
                    i=1;
                if (CX+i<SX)
                    CX+=i;
                else
                    CX=SX;
                L_CURSOR;
                vt->state=0;
                break;
                
            case 'A':	/* ESC[A -> move cursor up, no scrolling */
                i=vt->tok[0];
                if (i<1)
                    i=1;
                if (CY>i)
                    CY-=i;
                else
                    CY=0;
                L_CURSOR;
                vt->state=0;
                break;
                
            case 'B':	/* ESC[D -> move cursor down, no scrolling */
                i=vt->tok[0];
                if (i<1)
                    i=1;
                if (CY+i<SY)
                    CY+=i;
                else
                    CY=SY;
                L_CURSOR;
                vt->state=0;
                break;
            
            case 'r':	/* ESC[r -> set scrolling region */
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
                vt->state=0;
                break;
            
            case 'J':	/* ESC[J -> erase screen */
                switch(vt->tok[0])
                {
                case 0: /* from cursor */
                    CLEAR(CX, CY, SX*SY-(CY*SX+CX));
                    break;
                case 1: /* to cursor */
                    CLEAR(0, 0, CY*SX+CX);
                    break;
                case 2: /* whole screen */
                    CLEAR(0, 0, SX*SY);
                }
                vt->state=0;
                break;
                
            case 'K':	/* ESC[K -> erase line */
                switch(vt->tok[0])
                {
                case 0: /* from cursor */
                    CLEAR(CX, CY, SX-CX);
                    break;
                case 1: /* to cursor */
                    CLEAR(0, CY, CX);
                    break;
                case 2: /* whole line */
                    CLEAR(0, CY, SX);
                }
                vt->state=0;
                break;
            
            case 'f': case 'H':	/* ESC[f, ESC[H -> move cursor */
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
                vt->state=0;
                break;
            
            case 'G': case '`':	/* ESC[G, ESC[` -> move cursor horizontally */
                CX=vt->tok[0]-1;
                if (CX<0)
                    CX=0;
                else if (CX>=SX)
                    CX=SX-1;
                L_CURSOR;
                vt->state=0;
                break;
            
            case 'd':	/* ESC[d -> move cursor vertically */
                CY=vt->tok[0]-1;
                if (CY<0)
                    CY=0;
                else if (CY>=SY)
                    CY=SY-1;
                L_CURSOR;
                vt->state=0;
                break;
            
            case 'c':	/* ESC[c -> reset to power-on defaults */
                vt100_reset(vt);
                if (vt->l_clear)
                    vt->l_clear(vt, 0, 0, SX*SY);
                L_CURSOR;
                break;
            
            case 't':	/* ESC[t -> window manipulation */
                vt->state=0;
                switch(vt->tok[0])
                {
                case 8:	    /* ESC8;<h>;<w>t -> resize */
                    if (!vt->opt_allow_resize)
                        break;
                    while(vt->ntok<2)
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
            
        case 4:		/* ESC [ ? */
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
                for(i=0;i<=vt->ntok;i++)
		    switch(vt->tok[i])
		    {
		    case 7:
		        vt->opt_auto_wrap=1;
		        break;
                    case 25:
                        FLAG(VT100_FLAG_CURSOR, 1);
                        break;
#ifdef VT100_DEBUG
                    default:
                        printf("Unknown option: ?%u\n", vt->tok[i]);
#endif
                    }
                vt->state=0;
                break;
                
            case 'l':
                for(i=0;i<=vt->ntok;i++)
		    switch(vt->tok[i])
		    {
		    case 7:
		        vt->opt_auto_wrap=0;
		        break;
                    case 25:
                        FLAG(VT100_FLAG_CURSOR, 0);
                        break;
#ifdef VT100_DEBUG
                    default:
                        printf("Unknown option: ?%u\n", vt->tok[i]);
#endif
		    }
                vt->state=0;
                break;
                
            default:
                goto error;
            }
            break;

        case 5:	/* ESC ( -> set G0 charset */
            set_charset(vt, 0, *buf);
            vt->state=0;
            break;
            
        case 6:	/* ESC ) -> set G1 charset */
            set_charset(vt, 1, *buf);
            vt->state=0;
            break;
        
        case 7:	/* ESC % */
            switch(*buf)
            {
            case '@':	/* ESC % @ -> disable UTF-8 */
                vt->utf=0;
#ifdef VT100_DEBUG
                printf("Disabling UTF-8.\n");
#endif
                break;
            case '8':	/* ESC % 8, ESC % g -> enable UTF-8 */
            case 'G':
                vt->utf=1;
#ifdef VT100_DEBUG
                printf("Enabling UTF-8.\n");
#endif
                break;
            default:
                goto error;
            }
            vt->state=0;
            break;
            
        default:
            goto error;
        }
    }
}

#define MAX_PRINTABLE 16384

export void vt100_printf(vt100 vt, const char *fmt, ...)
{
    va_list ap;
    char buf[MAX_PRINTABLE];
    int len;
    
    va_start(ap, fmt);
    len=vsnprintf(buf,MAX_PRINTABLE,fmt,ap);
    va_end(ap);
    if (len<=0)
        return;
    if (len>MAX_PRINTABLE)
        vt100_write(vt, buf, MAX_PRINTABLE-1);
    else
        vt100_write(vt, buf, len);
}
