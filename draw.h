#undef RV	/* reverse video */

#define MAX_LINE 132

typedef struct { unsigned char r,g,b,a; } color;

extern void draw_vt(HDC dc, int px, int py, vt100 *vt);
extern void draw_init();
extern void draw_border(HDC dc, vt100 *vt);
        
extern int chx,chy;

#define clWoodenBurn 0x000F1728
#define clWoodenDown 0x004B5E6B
#define clWoodenMid  0x008ba9c3
#define clWoodenUp   0x009ECCE2
