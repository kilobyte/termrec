/*#define VT100_DEBUG*/

#define VT100_DEFAULT_CHARSET charset_cp437

typedef unsigned int ucs;

typedef struct 
{
    ucs ch;
    int attr;
} attrchar;

typedef unsigned short *charset;

#define VT100_MAXTOK 10

typedef struct
{
    int sx,sy;	           /* screen size */
    int s1,s2;             /* scrolling region */
    int cx,cy;             /* cursor position */
    int save_cx,save_cy;   /* saved cursor position */
    attrchar *scr;         /* screen buffer */
    int attr;              /* current attribute */
    charset G[2];          /* G0 and G1 charsets */
    charset transl;        /* current charset */
    int curG;              /* current charset as G# */
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
    int opt_allow_resize;  /* whether vt100 input is allowed to resize */
    int opt_auto_wrap;     /* ?7: auto wrap at right margin */
    int opt_cursor;        /* ?25: show/hide cursor */
    int opt_kpad;          /* keypad: application/numeric */
} vt100;

#define VT100_ATTR_BOLD		0x010000
#define VT100_ATTR_DIM		0x020000
#define VT100_ATTR_ITALIC	0x040000
#define VT100_ATTR_UNDERLINE	0x080000
#define VT100_ATTR_BLINK	0x100000
#define VT100_ATTR_INVERSE	0x200000

void vt100_init(vt100 *vt, int sx, int sy, int resizable, int utf);
int vt100_resize(vt100 *vt, int nsx, int nsy);
void vt100_reset(vt100 *vt);
void vt100_free(vt100 *vt);
void vt100_write(vt100 *vt, char *buf, int len);
void vt100_printf(vt100 *vt, const char *fmt, ...);
int vt100_copy(vt100 *vt, vt100 *nvt);
