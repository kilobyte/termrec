#include "config.h"
#include "tty.h"
#include "export.h"

#define R(x) ((uint8_t)((x)>>16))
#define G(x) ((uint8_t)((x)>> 8))
#define B(x) ((uint8_t) (x)     )

static uint32_t col256_to_rgb(uint32_t i)
{
    i &= 0xff;

    // Standard colours.
    if (i < 16)
    {
        if (i == 3)
            return 0xaa5500; // real CGA/VGA special-cases brown
        int c = (i&1 ? 0xaa0000 : 0x000000)
              | (i&2 ? 0x00aa00 : 0x000000)
              | (i&4 ? 0x0000aa : 0x000000);
        return (i >= 8) ? c + 0x555555 : c;
    }
    // 6x6x6 colour cube.
    else if (i < 232)
    {
        i -= 16;
        int r = i / 36, g = i / 6 % 6, b = i % 6;
        return (r ? r * 0x280000 + 0x370000 : 0)
             | (g ? g * 0x002800 + 0x003700 : 0)
             | (b ? b * 0x000028 + 0x000037 : 0);
    }
    // Grayscale ramp.
    else // i < 256
        return (i * 10 - 2312) * 0x010101;
}

static int sqrd(unsigned char a, unsigned char b)
{
    int _ = ((int)a) - ((int)b);
    return (_>=0)?_:-_;;
}

#if 0
// brightness formula
static uint32_t rgb_diff(uint32_t x, uint32_t y)
{
    return 2*sqrd(R(x), R(y))
         + 3*sqrd(G(x), G(y))
         + 1*sqrd(B(x), B(y));
}
#else
// Riermersma's formula
static uint32_t rgb_diff(uint32_t x, uint32_t y)
{
    int rm = (R(x)+R(y))/2;
    return 2*sqrd(R(x), R(y))
         + 4*sqrd(G(x), G(y))
         + 3*sqrd(B(x), B(y))
         + rm*(sqrd(R(x), R(y))-sqrd(B(x), B(y)))/256;
}
#endif

static uint32_t rgb_to_16(uint32_t c)
{
    uint8_t max = R(c);
    if (max < G(c))
        max = G(c);
    if (max < B(c))
        max = B(c);

    uint8_t hue = 0;
    if (R(c) > max/2+32)
        hue|=1;
    if (G(c) > max/2+32)
        hue|=2;
    if (B(c) > max/2+32)
        hue|=4;

    if (hue == 7 && max <= 0x70)
        return 8;

    return (max > 0xc0) ? hue|8 : hue;
}


static uint32_t m6(uint32_t x)
{
    x = (x+5)/40;
    return x? x-1 : 0;
}

static uint32_t rgb_to_color_cube(uint32_t c)
{
    return 16 + m6(R(c))*36 + m6(G(c))*6 + m6(B(c));
}

static uint32_t rgb_to_grayscale_gradient(uint32_t c)
{
    int x = 232 + (R(c)*2 + G(c)*3 + B(c)) / 60;
    return (x > 255)? 255 : x;
}

static uint32_t rgb_to_256(uint32_t c)
{
    uint32_t c1 = rgb_to_16(c);
    uint32_t bd = rgb_diff(c, col256_to_rgb(c1));
    uint32_t res = c1;

    uint32_t c2 = rgb_to_color_cube(c);
    uint32_t d = rgb_diff(c, col256_to_rgb(c2));
    if (d < bd)
        bd = d, res = c2;

    uint32_t c3 = rgb_to_grayscale_gradient(c);
    d = rgb_diff(c, col256_to_rgb(c3));
    if (d < bd)
        res = c3;
    return res;
}

export uint32_t tty_color_convert(uint32_t c, uint32_t to)
{
    if (!to)
        return 0;

    uint32_t from = (c&VT100_ATTR_COLOR_TYPE)>>24;
    if (to > 3)
        to>>=24;
    if (from == to)
        return c;
    if (from==1 && to==2)
        return c;
    if (from==1 || from==2)
        c = col256_to_rgb(c);
    switch (to)
    {
    case 1:
        return rgb_to_16(c);
    case 2:
        return rgb_to_256(c);
    case 3:
        return c;
    default:
        return 0;
    }
}
