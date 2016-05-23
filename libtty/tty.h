#ifndef _VT100_H
#define _VT100_H
#include <stdio.h>

typedef unsigned int ucs;

typedef struct
{
    ucs ch;
    int attr;
} attrchar;

#define VT100_MAXTOK 10

enum
{
    VT100_FLAG_CURSOR,		// visible cursor
    VT100_FLAG_KPAD,		// application keypad mode
    VT100_FLAG_AUTO_WRAP,	// auto wrap at right margin
};

typedef struct tty
{
    /*=[ basic data ]=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
    int sx,sy;	           // screen size
    int cx,cy;             // cursor position
    attrchar *scr;         // screen buffer
    int attr;              // current attribute
    /*=[ internal vt state ]=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
    int s1,s2;             // scrolling region
    int save_cx,save_cy;   // saved cursor position
    int save_attr;         // saved attribute
    unsigned int G:2;      // bitfield: do G0 and G1 use vt100 graphics?
    unsigned int curG:1;   // current G charset
    unsigned int save_G:2; // saved G, curG
    unsigned int save_curG:1;
    // UTF-8 state
    ucs utf_char;
    ucs utf_surrogate;
    int utf_count;
    // parser state
    int ntok;
    int tok[VT100_MAXTOK];
    int state;
    // flags
    int cp437 :1;               // non-UTF-8
    int allow_resize :1;	// is input allowed to resize?
    int opt_auto_wrap :1;	// ?7: auto wrap at right margin
    int opt_cursor :1;		// ?25: show/hide cursor
    int opt_kpad :1;		// keypad: application/numeric
    /*=[ listeners ]=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
    void *l_data;
        // any private data
    void (*l_char)(struct tty *vt, int x, int y, ucs ch, int attr);
        // after a character has been written to the screen
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
    void (*l_resize)(struct tty *vt, int sx, int sy);
        // after the terminal has been resized
    void (*l_flush)(struct tty *vt);
        // when a write chunk ends
    void (*l_free)(struct tty *vt);
        // before the terminal is destroyed
} *tty;

#define VT100_ATTR_BOLD		0x010000
#define VT100_ATTR_DIM		0x020000
#define VT100_ATTR_ITALIC	0x040000
#define VT100_ATTR_UNDERLINE	0x080000
#define VT100_ATTR_BLINK	0x100000
#define VT100_ATTR_INVERSE	0x200000

tty	tty_init(int sx, int sy, int resizable);
int	tty_resize(tty vt, int nsx, int nsy);
void	tty_reset(tty vt);
void	tty_free(tty vt);
void	tty_write(tty vt, char *buf, int len);
void	tty_printf(tty vt, const char *fmt, ...) \
	    __attribute__((format (printf, 2, 3)));
tty	tty_copy(tty vt);

void	vtvt_dump(tty vt);
void	vtvt_resize(tty vt, int sx, int sy);
void	vtvt_attach(tty vt, FILE *f, int dump);
#endif
