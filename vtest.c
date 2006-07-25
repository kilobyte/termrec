#include <stdio.h>
#include <wchar.h>
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

void tl_resize(vt100 *vt, int sx, int sy)
{
    printf("== resize to %dx%d\n", sx, sy);
}

void tl_free(vt100 *vt)
{
    printf("* size=%dx%d\n", vt->sx, vt->sy);
    printf("== free\n");
}

int main()
{
    vt100 vt;
    
    vt100_init(&vt);
    vt.l_char=tl_char;
    vt.l_cursor=tl_cursor;
    vt.l_clear=tl_clear;
    vt.l_scroll=tl_scroll;
    vt.l_resize=tl_resize;
    vt.l_free=tl_free;
    vt100_resize(&vt, 20, 5);
    
    vt100_printf(&vt, "abc\ndef\nghi\n");
    vt100_printf(&vt, "\e[%2$d;%1$dfblah", 20, 4);
    vt100_printf(&vt, "\e[%2$d;%1$df", 1, 5);
    vt100_printf(&vt, "abc\ndef\nghi\n");
    
    vt100_printf(&vt, "\e[8;4;20t");
    
    vt100_free(&vt);
    
    return 0;
}
