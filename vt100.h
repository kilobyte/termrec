typedef struct 
{
    int ch;
    int attr;
} attrchar;

#define VT100_MAXTOK 10

typedef struct
{
    int sx,sy;
    int s1,s2;
    int cx,cy;
    attrchar *scr;
    int attr;
    int ntok;
    int tok[VT100_MAXTOK];
    int state;
} vt100;

#define VT100_ATTR_BOLD		0x010000
#define VT100_ATTR_DIM		0x020000
#define VT100_ATTR_ITALIC	0x040000
#define VT100_ATTR_UNDERLINE	0x080000
#define VT100_ATTR_BLINK	0x100000
#define VT100_ATTR_INVERSE	0x200000

void vt100_init(vt100 *vt);
int vt100_resize(vt100 *vt, int nsx, int nsy);
void vt100_free(vt100 *vt);
// void vt100_scroll(vt100 *vt, int nl);
void vt100_write(vt100 *vt, char *buf, int len);
void vt100_printf(vt100 *vt, const char *fmt, ...);
