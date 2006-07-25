typedef unsigned int ucs;

typedef struct 
{
    ucs ch;
    int attr;
} attrchar;

typedef unsigned short *charset;

#define VT100_MAXTOK 10

#define VT100_FLAG_CURSOR	1	/* visible cursor */
#define VT100_FLAG_KPAD		2	/* application keypad mode */

typedef struct vt100
{
    int sx,sy;	           /* screen size */
    int s1,s2;             /* scrolling region */
    int cx,cy;             /* cursor position */
    int save_cx,save_cy;   /* saved cursor position */
    attrchar *scr;         /* screen buffer */
    int attr;              /* current attribute */
    charset G[2];          /* G0 and G1 charsets */
    charset transl;        /* current charset */
    int chs;               /* current charset as G# */
    /* UTF-8 state */
    int utf;               /* UTF-8 on/off */
    ucs utf_char;
    ucs utf_surrogate;
    int utf_count;
    /* parser state */
    int ntok;
    int tok[VT100_MAXTOK];
    int state;
    /* flags */
    int opt_auto_wrap;     /* ?7: auto wrap at right margin */
#define opt_cursor flags[VT100_FLAG_CURSOR]	/* ?25: show/hide cursor */
#define opt_kpad flags[VT100_FLAG_KPAD]	/* keypad: application/numeric */
    int flags[10];
    /*=[ listeners ]=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
    void *l_data;
        /* any private data */
    void *l_changes;
        /* pending changes in your private format */
    void (*l_char)(struct vt100 *vt, int x, int y, ucs ch, int attr);
        /* after a character has been written to the screen */
    void (*l_cursor)(struct vt100 *vt, int x, int y);
        /* after the cursor has moved */
    void (*l_clear)(struct vt100 *vt, int x, int y, int len);
        /* after a chunk of screen has been cleared
            * len can spill into subsequent lines
           If the cursor moves, you'll get a separate l_cursor, although
           it is already in place during the l_clear call. */
    void (*l_scroll)(struct vt100 *vt, int nl);
        /* after the region s1<=y<s2 is scrolled nl lines
            * nl<0 -> scrolling backwards
            * nl>0 -> scrolling forwards
           The cursor is already moved.	*/
    void (*l_flag)(struct vt100 *vt, int f, int v);
        /* when a flag changes to v */
    void (*l_resize)(struct vt100 *vt, int sx, int sy);
        /* after the tty has been resized */
    void (*l_free)(struct vt100 *vt);
        /* before the tty is destroyed */
} vt100;

#define VT100_ATTR_BOLD		0x010000
#define VT100_ATTR_DIM		0x020000
#define VT100_ATTR_ITALIC	0x040000
#define VT100_ATTR_UNDERLINE	0x080000
#define VT100_ATTR_BLINK	0x100000
#define VT100_ATTR_INVERSE	0x200000

void vt100_init(vt100 *vt);
int vt100_resize(vt100 *vt, int nsx, int nsy);
void vt100_reset(vt100 *vt);
void vt100_free(vt100 *vt);
void vt100_write(vt100 *vt, char *buf, int len);
void vt100_printf(vt100 *vt, const char *fmt, ...);
int vt100_copy(vt100 *vt, vt100 *nvt);
