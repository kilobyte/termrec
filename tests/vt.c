#include <stdio.h>
#include <wchar.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tty.h"
#include "wcwidth.h"

static void tl_char(tty vt, int x, int y, ucs ch, int attr, int width)
{
    if (width == 1)
        printf("== char U+%04X attr=%X at %d,%d\n", ch, attr, x, y);
    else if (width == 2)
        printf("== char U+%04X attr=%X at %d,%d, CJK\n", ch, attr, x, y);
    else
        printf("== char U+%04X attr=%X at %d,%d, bad width %d!\n", ch, attr, x, y, width);
}

static void tl_comb(tty vt, int x, int y, ucs ch, int attr)
{
    printf("== comb U+%04X attr=%X at %d,%d\n", ch, attr, x, y);
}

static void tl_cursor(tty vt, int x, int y)
{
    printf("== cursor to %d,%d\n", x, y);
}

static void tl_clear(tty vt, int x, int y, int len)
{
    printf("== clear from %d,%d len %d\n", x, y, len);
}

static void tl_scroll(tty vt, int nl)
{
    printf("== scroll by %d lines\n", nl);
}

static void tl_flag(tty vt, int f, int v)
{
    printf("== flag %d (%s) set to %d\n", f,
        (v==VT100_FLAG_CURSOR)?"cursor visibility":
        (v==VT100_FLAG_KPAD)?"keypad mode":
        "unknown", v);
}

static void tl_resize(tty vt, int sx, int sy)
{
    printf("== resize to %dx%d\n", sx, sy);
}

static void tl_bell(tty vt)
{
    printf("== bell\n");
}

static void tl_free(tty vt)
{
    printf("== free\n");
}

static void dump(tty vt)
{
    int x,y,attr;

    printf(".-===[ %dx%d ]\n", vt->sx, vt->sy);
    attr=0x1010;
    for (y=0; y<vt->sy; y++)
    {
        printf("| ");
        for (x=0; x<vt->sx; x++)
        {
#define SCR vt->scr[x+y*vt->sx]
            if (SCR.attr!=attr)
                printf("{%X}", attr=SCR.attr);
            if (SCR.ch>=' ' && SCR.ch<127)
                printf("%c", SCR.ch);
            else if (SCR.ch == VT100_CJK_RIGHT)
                printf("[CJK]");
            else
                printf("[%04X]", SCR.ch);
            if (SCR.comb)
            {
                int ci=SCR.comb;
                char pref='<';
                while (ci)
                {
                    printf("%c%04X", pref, vt->combs[ci].ch);
                    pref=' ';
                    ci=vt->combs[ci].next;
                }
                printf(">");
            }
        }
        printf("\n");
    }
    printf("`-===[ cursor at %d,%d ]\n", vt->cx, vt->cy);
}

#define SX vt->sx
#define SY vt->sy
#define CX vt->cx
#define CY vt->cy
#define FREECOMB vt->combs[0].next
#define NCOMBS   vt->combs[0].ch

static const char* validate(tty vt)
{
    if (SX<2 || SY<1)
        return "screen too small";
    if (CX<0 || CX>SX || CY<0 || CY>=SY)
        return "cursor out of bounds";

    int SS=SX*SY;
    if (!vt->combs)
    {
        for (int i=0;i<SS;i++)
            if (vt->scr[i].comb)
                return "combining when there should be none";
    }
    else
    {
        uint8_t *tc=calloc(NCOMBS, 1);
        uint32_t j;
        if (!tc)
            return "OUT OF MEMORY";
        tc[0]=1;
        // check used chains
        for (int i=0;i<SS;i++)
            for (j=vt->scr[i].comb; j; j=vt->combs[j].next)
            {
                if (j>=NCOMBS)
                    return free(tc), "combining out of table";
                if (tc[j])
                    return free(tc), "combining chain loop";
                tc[j]=1;
            }
        // check free space
        for (j=FREECOMB; j<NCOMBS; j=vt->combs[j].next)
        {
            if (tc[j])
                return free(tc), "free combining used or looped";
            tc[j]=1;
        }
        if (j!=NCOMBS)
            return free(tc), "bad termination of free combining chain";
        for (j=0; j<NCOMBS; j++)
            if (!tc[j])
                return free(tc), "combining slot neither used nor free";
        free(tc);
    }

    for (int y=0;y<SY;y++)
        for (int x=0;x<SX;x++)
        {
            attrchar *ac = &vt->scr[y*SX+x];
            switch (mk_wcwidth(ac->ch))
            {
            case 0:
                if (ac->ch)
                    return "width 0 character as primary";
                break;
            case 1:
                if (ac->ch == VT100_CJK_RIGHT)
                {
                    if (!(ac->attr & VT100_ATTR_CJK))
                        return "right half but attr says not CJK";
                    if (x<=0)
                        return "right half at left edge of screen";
                    if (!(ac[-1].attr & VT100_ATTR_CJK))
                        return "right half but prev attr not CJK";
                }
                else if (ac->attr & VT100_ATTR_CJK)
                    return "width 1 but attr says CJK";
                break;
            case 2:
                if (!(ac->attr & VT100_ATTR_CJK))
                    return "width 2 but attr says not CJK";
                if (x+1>=SX)
                    return "width 2 at right edge of screen";
                if (ac[1].ch != VT100_CJK_RIGHT)
                    return "width 2 but no right half";
                break;
            default:
                return "control character on screen";
            }
        }

    return 0;
}

#define BUFFER_SIZE 65536

int main(int argc, char **argv)
{
    tty vt;
    char buf[BUFFER_SIZE];
    int len;
    bool dump_flag=false;

    vt = tty_init(20, 5, 1);

    while (1)
        switch (getopt(argc, argv, "ed"))
        {
        case -1:
            goto run;
        case ':':
        case '?':
            exit(1);
        case 'e':
            vt->l_char=tl_char;
            vt->l_comb=tl_comb;
            vt->l_cursor=tl_cursor;
            vt->l_clear=tl_clear;
            vt->l_scroll=tl_scroll;
            vt->l_flag=tl_flag;
            vt->l_resize=tl_resize;
            vt->l_bell=tl_bell;
            vt->l_free=tl_free;
            break;
        case 'd':
            dump_flag=true;
        }
run:
    while ((len=read(0, buf, BUFFER_SIZE))>0)
    {
        tty_write(vt, buf, len);
        const char *inv=validate(vt);
        if (inv)
        {
            printf("!!! tty invalid: %s !!!\n", inv);
            break;
        }
    }

    if (dump_flag)
        dump(vt);

    tty_free(vt);

    return 0;
}
