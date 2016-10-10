#include <stdio.h>
#include <wchar.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tty.h"

static void tl_char(tty vt, int x, int y, ucs ch, int attr)
{
    printf("== char U+%04X attr=%X at %d,%d\n", ch, attr, x, y);
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
            else
                printf("[%04X]", SCR.ch);
        }
        printf("\n");
    }
    printf("`-===[ cursor at %d,%d ]\n", vt->cx, vt->cy);
}

#define BUFFER_SIZE 65536

int main(int argc, char **argv)
{
    tty vt;
    char buf[BUFFER_SIZE];
    int len;
    bool dump_flag=false;

    vt = tty_init(20, 5, 0);

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
            vt->l_cursor=tl_cursor;
            vt->l_clear=tl_clear;
            vt->l_scroll=tl_scroll;
            vt->l_flag=tl_flag;
            vt->l_resize=tl_resize;
            vt->l_free=tl_free;
            break;
        case 'd':
            dump_flag=true;
        }
run:
    while ((len=read(0, buf, BUFFER_SIZE))>0)
        tty_write(vt, buf, len);

    if (dump_flag)
        dump(vt);

    tty_free(vt);

    return 0;
}
