#undef RV	/* reverse video */

#define MAX_LINE 132

typedef struct { unsigned char r,g,b,a; } color;

extern void draw_vt(HDC dc, int px, int py, vt100 *vt);
extern void draw_init();
extern int chx,chy;
