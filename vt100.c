#include "vt100.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#define SX vt->sx
#define SY vt->sy
#define CX vt->cx
#define CY vt->cy

void vt100_init(vt100 *vt)
{
    memset(vt,0,sizeof(vt100));
    vt->attr=0xffff;
}

int vt100_resize(vt100 *vt, int nsx, int nsy)
{
    int x,y;
    attrchar *nscr;

    if (nsx==SX && nsy==SY)
        return 0;
    printf("Resize from %dx%d to %dx%d\n", SX, SY, nsx, nsy);
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

void vt100_free(vt100 *vt)
{
    free(vt->scr);
}

static void vt100_clear_region(vt100 *vt, int st, int l)
{
    attrchar *c;

    c=vt->scr+st;
    while(l--)
    {
        c->ch=' ';
        c++->attr=vt->attr;
    }
}

static void vt100_scroll(vt100 *vt, int nl)
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

void vt100_write(vt100 *vt, char *buf, int len)
{
    int i;

    while(len--)
    {
        switch(vt->state)
        {
        error:
            vt->state=0;
        case 0:
            switch(*buf)
            {
            case 0:
                break;
            case 8:
                if (CX)
                    CX--;
                break;
            case 9:
                if (CX<SX)
                    do
                    {
                        vt->scr[CY*SX+CX].attr=vt->attr;
                        vt->scr[CY*SX+CX].ch=' ';
                        CX++;
                    } while(CX<SX && CX&7);
                break;
            case 10:
                CX=0;
                CY++;
                if (CY>=SY)
                {
                    vt100_scroll(vt, 1);
                    CY=SY-1;
                }
                break;
            case 13:
                CX=0;
                break;
            case 27:
                vt->state=1;
                break;
            default:
                if (CX>=SX)
                {
                    CX=0;
                    CY++;
                    if (CY>=SY)
                    {
                        vt100_scroll(vt, 1);
                        CY=SY-1;
                    }
                }
                vt->scr[CY*SX+CX].ch=*buf;
                vt->scr[CY*SX+CX].attr=vt->attr;
                CX++;
            }
            break;
        case 1:		/* ESC */
            switch(*buf)
            {
            case '[':
                vt->state=2;
                vt->ntok=0;
                vt->tok[0]=0;
                break;
            default:
                goto error;
            }
            break;
        case 2:		/* ESC [ */
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
            case 'm':
                for(i=0;i<=vt->ntok;i++)
                    switch(vt->tok[i])
                    {
                    case 0:
                        vt->attr=0xffff;
                        break;
                    case 1:
                        vt->attr|=VT100_ATTR_BOLD;
                        break;
                    case 2:
                        vt->attr|=VT100_ATTR_DIM;
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
                        vt->attr&=~VT100_ATTR_BOLD;
                        break;
                    case 22:
                        vt->attr&=~VT100_ATTR_DIM;
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
            default:
                goto error;
            }
            break;
        default:
            goto error;
        }
        buf++;
    }
}

#define MAX_PRINTABLE 16384

void vt100_printf(vt100 *vt, const char *fmt, ...)
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
