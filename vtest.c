#include <stdio.h>
#include <wchar.h>
#include <unistd.h>
#include <stdlib.h>
#include "vt100.h"

void tl_char(vt100 *vt, int x, int y, ucs ch, int attr)
{
    printf("== char U+%04X attr=%X at %d,%d\n", ch, attr, x, y);
}

void tl_cursor(vt100 *vt, int x, int y)
{
    printf("== cursor to %d,%d\n", x, y);
}

void tl_clear(vt100 *vt, int x, int y, int len)
{
    printf("== clear from %d,%d len %d\n", x, y, len);
}

void tl_scroll(vt100 *vt, int nl)
{
    printf("== scroll by %d lines\n", nl);
}

void tl_flag(vt100 *vt, int f, int v)
{
    printf("== flag %d (%s) set to %d\n", f,
        (v==VT100_FLAG_CURSOR)?"cursor visibility":
        (v==VT100_FLAG_KPAD)?"keypad mode":
        "unknown", v);
}

void tl_resize(vt100 *vt, int sx, int sy)
{
    printf("== resize to %dx%d\n", sx, sy);
}

void tl_free(vt100 *vt)
{
    printf("== free\n");
}

void dump(vt100 *vt)
{
    int x,y,attr;
    
    printf(".-===[ %dx%d ]\n", vt->sx, vt->sy);
    attr=0xFFFF;
    for(y=0; y<vt->sy; y++)
    {
        printf("| ");
        for(x=0; x<vt->sx; x++)
        {
#define SCR vt->scr[x+y*vt->sx]
            if (SCR.attr!=attr)
                printf("{%X}", attr=SCR.attr);
            if (SCR.ch>=' ' && SCR.ch<127)
                printf("%c", SCR.ch);
            else
                printf("[%04X]", SCR.ch);
        }
        printf("\n");
    }
    printf("`-===[ cursor at %d,%d]\n", vt->cx, vt->cy);
}

int crap=0;

int main(int argc, char **argv)
{
    vt100 vt;
    
    vt100_init(&vt, 20, 5, 0, 1);
    
    while(1)
        switch(getopt(argc, argv, "ed"))
        {
        case -1:
            goto run;
        case ':':
        case '?':
            exit(1);
        case 'e':
            vt.l_char=tl_char;
            vt.l_cursor=tl_cursor;
            vt.l_clear=tl_clear;
            vt.l_scroll=tl_scroll;
            vt.l_flag=tl_flag;
            vt.l_resize=tl_resize;
            vt.l_free=tl_free;
            break;
        case 'd':
            crap=1;
        }
run:

    vt100_printf(&vt, "abc\ndef\nghi\n");
    vt100_printf(&vt, "\e[%2$d;%1$dfblah", 20, 4);
    vt100_printf(&vt, "\e[%2$d;%1$df", 1, 5);
    vt100_printf(&vt, "abc\ndef\nghi\n");
    
    vt100_printf(&vt, "\e[8;4;20t");
    if (crap)
        dump(&vt);
    
    vt100_free(&vt);
    
    return 0;
}
