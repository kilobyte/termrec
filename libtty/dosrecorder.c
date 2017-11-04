#include "config.h"
#ifdef HAVE_ZLIB_H
# include <zlib.h>
#else
# ifdef SHIPPED_LIBZ
#  include SHIPPED_LIBZ_H
# endif
#endif
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "_stdint.h"
#include "gettext.h"
#include "export.h"
#include "formats.h"
#include "charsets.h"

#if (defined HAVE_LIBZ) || (SHIPPED_LIBZ)
// format: dosrecorder

#define MAXSCREENS 256
typedef struct { char c,a; } attrchar, screen[25][80];

static char rgbbgr[8]="04261537";

#define bp (*b)
static inline void setattr(attrchar *ch, int *oattr, char **b)
{
    if (*oattr!=ch->a)
    {
        *oattr=ch->a;
        *bp++='\e', *bp++='[', *bp++='0';
        if (ch->a&0x80) // blink
            *bp++=';', *bp++='5';
        *bp++=';', *bp++='4', *bp++=rgbbgr[(ch->a>>4)&7]; // background
        if (ch->a&0x08) // brightness
            *bp++=';', *bp++='1';
        *bp++=';', *bp++='3', *bp++=rgbbgr[ch->a&7]; // foreground
        *bp++='m';
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#ifdef __clang__
# pragma GCC diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#endif
static inline void wrchar(attrchar *ch, char **b)
{
    // Unicode is 32-bit, charset_cp437[] only 16-bit as all its entries fit
    // into the range.  Thus, hush the compiler warning.
    TF8(bp, charset_cp437[(unsigned char)ch->c]);
}
#pragma GCC diagnostic pop
#undef bp

static inline int cnt_spaces(attrchar *ch, int a, int max)
{
    int i=0;

    while (ch->c==' ' && ch->a==a && max--)
        i++, ch++;
    return i;
}

#define BUFFER_SIZE 65536
/* Max frame size is 34166: 25*80 cells of an UTF-8 char (max 3 bytes) and attrs
   (max 14 bytes), plus cursor movements, one per line plus one per >=10 symbols
   skipped.  Rounding up to 64KB just to be safe.
*/
#define MINJUMP 10 // when to jump instead of overwriting same text
#define MINCL 20 // when to prefer clears over writing spaces
static int scrdiff(screen *vt, screen *scr, char *buf)
{
    char *bp=buf;
    int x,y;
    int cx=-1,cy=-1;
    int attr=-1;
    int sp;

    for (y=0; y<25; y++)
        for (x=0; x<80; x++)
            if (vt[y][x]!=scr[y][x])
            {
                if (y!=cy || cx+MINJUMP<x)
                    bp+=sprintf(bp, "\e[%d;%df", (cy=y)+1, cx=x+1);
                else
                {
                    while (cx<x)
                    {
                        setattr(&(*scr)[y][cx], &attr, &bp);
                        wrchar(&(*scr)[y][cx], &bp);
                        cx++;
                    }
                    cx++;
                }
                setattr(&(*scr)[y][x], &attr, &bp);
                if (cx>80-MINCL || (sp=cnt_spaces(&(*scr)[y][x], attr, 80-cx))<MINCL)
                    wrchar(&(*scr)[y][x], &bp);
                else
                {
                    bp+=sprintf(bp, "\e[%dX", sp);
                    x+=sp-1;
                }
            }

    return bp-buf;
}

void play_dosrecorder(FILE *f,
    void (*synch_init_wait)(const struct timespec *ts, void *arg),
    void (*synch_wait)(const struct timespec *tv, void *arg),
    void (*synch_print)(const char *buf, int len, void *arg),
    void *arg, const struct timespec *cont)
{
    gzFile g;
    screen vt, scr, screens[MAXSCREENS];
    int i, x, y, note;
#pragma pack(push)
#pragma pack(1)
    struct ch
    {
        uint16_t pos,len;
    } chs[25*80];
    struct
    {
        unsigned char w, h, x, y;
        unsigned char titlelen;
        char attr;
        char flags;
    } nh;
    struct
    {
        uint32_t delay;
        unsigned char cx, cy;
        unsigned char sscr, dscr;
        uint16_t nchunks;
    } fh;
#pragma pack(pop)
    char buf[BUFFER_SIZE], *bp;
    struct timespec tv;

    if (!(g=gzdopen(dup(fileno(f)), "rb")))
    {
        synch_print(buf, snprintf(buf, BUFFER_SIZE, _("Not a DosRecorder recording!")), arg);
        return;
    }
    memset(vt, 0, sizeof(screen));
    for (i=0; i<MAXSCREENS; i++)
        for (y=0; y<25; y++)
            for (x=0; x<80; x++)
                screens[i][y][x].c=' ', screens[i][y][x].a=7;

    bp=buf+sprintf(buf, "\ec\e%%G\e[8;25;80t");
    while (gzread(g, &fh, sizeof(fh))==sizeof(fh))
    {
        note=fh.nchunks&0x8000;
        memcpy(&scr, &screens[fh.sscr], sizeof(screen));
        if ((fh.nchunks&=0x7fff)>25*80)
            goto end;   // corrupted file -- too many chunks
        if (gzread(g, chs, fh.nchunks*sizeof(struct ch))
                   !=(int)(fh.nchunks*sizeof(struct ch)))
            goto end;
        for (i=0; i<fh.nchunks; i++)
        {
            if (chs[i].pos>=25*80 || chs[i].len+chs[i].pos>25*80)
                goto end;       // corrupted file
            gzread(g, &scr[0][0]+chs[i].pos, chs[i].len*sizeof(attrchar));
        }
        memcpy(&screens[fh.dscr], &scr, sizeof(screen));

        if (note)
        {
            if (gzread(g, &nh, sizeof(nh)!=sizeof(nh)))
                goto end;
            for (i=0; i<nh.h; i++)
                gzread(g, &scr[nh.y+i][nh.x], nh.w*sizeof(attrchar));
            if (nh.titlelen>80)
                goto end;
            *bp++='\e', *bp++=']', *bp++='0', *bp++=';';
            gzread(g, bp, nh.titlelen);
            bp+=nh.titlelen;
            *bp++=7;
        }

        bp+=scrdiff(&vt, &scr, bp);

        tv.tv_sec=fh.delay/1000;
        tv.tv_nsec=(fh.delay-tv.tv_sec*1000)*1000000;
        synch_wait(&tv, arg);
        synch_print(buf, bp-buf, arg);

        bp=buf;
    }

end:
    gzclose(g);
}
#endif
