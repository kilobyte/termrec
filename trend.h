typedef struct { unsigned char b,g,r,a; } pixel;
typedef pixel grayp;

extern grayp *trend_data;
extern int chx,chy;

#define blend(c1,c2,a) ((255+((unsigned int)c1)*a+((unsigned int)c2)*(255-a))>>8)

void init_trend(int nx, int ny);
void learn_char(pixel *pic, char ch, int psx, int psy, int px, int py);
void draw_char(pixel *pic, char ch, int psx, int psy, int px, int py, pixel fg, pixel bg);
void tile_pic(pixel *pic, int psx, int psy, int px, int py, int sx, int sy, pixel *src, int ssx, int ssy);
void draw_pic(pixel *pic, int psx, int psy, int px, int py, pixel *src, int ssx, int ssy);
void overlay_pic(pixel *pic, int psx, int psy, int px, int py, pixel *src, int ssx, int ssy, int alpha);
