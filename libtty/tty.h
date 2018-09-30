#ifndef _VT100_H
#define _VT100_H
#include <stdio.h>
#include <stdint.h>

typedef uint32_t ucs;

typedef struct
{
    ucs ch;
    uint32_t comb;
    uint64_t attr;
} attrchar;

typedef struct
{
    ucs ch;
    uint32_t next;
} combc;

#define VT100_MAXTOK 16

enum
{
    VT100_FLAG_CURSOR,          // visible cursor
    VT100_FLAG_KPAD,            // application keypad mode
    VT100_FLAG_AUTO_WRAP,       // auto wrap at right margin
};

#define VT100_CJK_RIGHT 0xffffffff /* ch value of right-half of a CJK char */

#define VT100_ATTR_COLOR_MASK 0x3ffffff
#define VT100_ATTR_COLOR_TYPE 0x3000000

#define VT100_COLOR_OFF 0
#define VT100_COLOR_16  1
#define VT100_COLOR_256 2
#define VT100_COLOR_RGB 3

#define VT100_ATTR_BOLD         0x04000000
#define VT100_ATTR_DIM          0x08000000
#define VT100_ATTR_ITALIC       0x10000000
#define VT100_ATTR_UNDERLINE    0x20000000
#define VT100_ATTR_STRIKE       0x40000000
#define VT100_ATTR_INVERSE      0x0400000000000000
#define VT100_ATTR_BLINK        0x0800000000000000
#define VT100_ATTR_CJK          0x8000000000000000

typedef struct tty
{
    /*=[ basic data ]=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
    int sx,sy;             // screen size
    int cx,cy;             // cursor position
    attrchar *scr;         // screen buffer
    combc *combs;          // combining character chains
    uint64_t attr;         // current attribute
    char *title;           // window title
    /*=[ internal vt state ]=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
    int s1,s2;             // scrolling region
    int save_cx,save_cy;   // saved cursor position
    uint64_t save_attr;    // saved attribute
    unsigned int G:2;      // bitfield: do G0 and G1 use vt100 graphics?
    unsigned int curG:1;   // current G charset
    unsigned int save_G:2; // saved G, curG
    unsigned int save_curG:1;
    // flags
    int cp437 :1;          // non-UTF-8
    int allow_resize :1;   // is input allowed to resize?
    int opt_auto_wrap :1;  // ?7: auto wrap at right margin
    int opt_cursor :1;     // ?25: show/hide cursor
    int opt_kpad :1;       // keypad: application/numeric
    // UTF-8 state
    ucs utf_char;
    ucs utf_surrogate;
    int utf_count;
    // parser state
    int state;
    int ntok;
    uint32_t tok[VT100_MAXTOK];
    char *oscbuf;
    int osclen;            // length of current osc command
    /*=[ listeners ]=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
    void *l_data;
        // any private data
    void (*l_char)(struct tty *vt, int x, int y, ucs ch, uint64_t attr, int width);
        // after a character has been written to the screen
    void (*l_comb)(struct tty *vt, int x, int y, ucs ch, uint64_t attr);
        // after a combining character has been added
    void (*l_cursor)(struct tty *vt, int x, int y);
        // after the cursor has moved
    void (*l_clear)(struct tty *vt, int x, int y, int len);
        // after a chunk of screen has been cleared
        // If an endpoint spills outside of the current line, it
        // must go all the way to an end of screen.
        // If the cursor moves, you'll get a separate l_cursor, although
        // it is already in place during the l_clear call.
    void (*l_scroll)(struct tty *vt, int nl);
        // after the region s1<=y<s2 is scrolled nl lines
        //  * nl<0 -> scrolling backwards
        //  * nl>0 -> scrolling forwards
        // The cursor is already moved.
    void (*l_flag)(struct tty *vt, int f, int v);
        // when a flag changes to v
    void (*l_osc)(struct tty *vt, int cmd, const char *str);
        // string command (window title, etc)
    void (*l_resize)(struct tty *vt, int sx, int sy);
        // after the terminal has been resized
    void (*l_flush)(struct tty *vt);
        // when a write chunk ends
    void (*l_bell)(struct tty *vt);
        // when an \a is received
    void (*l_free)(struct tty *vt);
        // before the terminal is destroyed
} *tty;

tty     tty_init(int sx, int sy, int resizable);
int     tty_resize(tty vt, int nsx, int nsy);
void    tty_reset(tty vt);
void    tty_free(tty vt);
void    tty_write(tty vt, const char *buf, int len);
void    tty_printf(tty vt, const char *fmt, ...) \
            __attribute__((format (printf, 2, 3)));
tty     tty_copy(tty vt);
uint32_t tty_color_convert(uint32_t c, uint32_t to);

void    vtvt_dump(tty vt);
void    vtvt_resize(tty vt, int sx, int sy);
void    vtvt_attach(tty vt, FILE *f, int dump);
#endif
