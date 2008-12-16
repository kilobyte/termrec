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
/* format: dosrecorder */

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
        if (ch->a&0x80) /* blink */
            *bp++=';', *bp++='5';
        *bp++=';', *bp++='4', *bp++=rgbbgr[(ch->a>>4)&7]; /* background */
        if (ch->a&0x08) /* brightness */
            *bp++=';', *bp++='1';
        *bp++=';', *bp++='3', *bp++=rgbbgr[ch->a&7]; /* foreground */
        *bp++='m';
    }
}

static inline void wrchar(attrchar *ch, int *oattr, char **b)
{
    tf8(bp, charset_cp437[(unsigned char)ch->c]);
}
#undef bp

static inline int sp_string(attrchar *ch, int a, int max)
{
    int i=0;
    
    while(ch->c==' ' && ch->a==a && max--)
        i++, ch++;
    return i;
}

#define BUFFER_SIZE 65536
/* Note: no checks if buf is big enough.  We won't produce more than 25*80 symbols, 
   each consisting of an UTF-8 char (max 3 bytes) and attrs, plus cursor movements,
   one per line plus one per >=10 symbols skipped.
   Thus, don't skimp on BUFFER_SIZE.  16k should be more than enough, but it's better
   to waste memory than have a buf overflow.
*/
static int scrdiff(screen *tty, screen *scr, char *buf)
{
    char *bp=buf;
    int x,y;
    int cx=-1,cy=-1;
    int attr=-1;
    int sp;
    
    for(y=0; y<25; y++)
        for(x=0; x<80; x++)
            if (tty[y][x]!=scr[y][x])
            {
                if (y!=cy || cx+10<x)
                    bp+=sprintf(bp, "\e[%d;%df", (cy=y)+1, cx=x+1);
                else
                {
                    while(cx<x)
                    {
                        setattr(&(*scr)[y][cx], &attr, &bp);
                        wrchar(&(*scr)[y][cx], &attr, &bp);
                        cx++;
                    }
                    cx++;
                }
                setattr(&(*scr)[y][x], &attr, &bp);
                if (cx>80-15 || (sp=sp_string(&(*scr)[y][x], attr, 80-cx))<15)
                    wrchar(&(*scr)[y][x], &attr, &bp);
                else
                {
                    bp+=sprintf(bp, "\e[%dX", sp);
                    x+=sp-1;
                }
            }
    
    return bp-buf;
}

void play_dosrecorder(FILE *f,
    void *(synch_init_wait)(struct timeval *ts, void *arg),
    void *(synch_wait)(struct timeval *tv, void *arg),
    void *(synch_print)(char *buf, int len, void *arg),
    void *arg, struct timeval *cont)
{
    gzFile g;
    screen tty, scr, screens[MAXSCREENS];
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
    struct timeval tv;
    
    if (!(g=gzdopen(dup(fileno(f)), "rb")))
    {
        synch_print(buf, snprintf(buf, BUFFER_SIZE, _("Not a DosRecorder recording!")), arg);
        return;
    }
    memset(tty, 0, sizeof(screen));
    for(i=0; i<MAXSCREENS; i++)
        for(y=0; y<25; y++)
            for(x=0; x<80; x++)
                screens[i][y][x].c=' ', screens[i][y][x].a=7;
    
    bp=buf+sprintf(buf, "\ec\e%%G");
    while(gzread(g, &fh, sizeof(fh))==sizeof(fh))
    {
        note=fh.nchunks&0x8000;
        memcpy(&scr, &screens[fh.sscr], sizeof(screen));
        if ((fh.nchunks&=0x7fff)>25*80)
            goto end;	/* corrupted file -- too many chunks */
        if (gzread(g, chs, fh.nchunks*sizeof(struct ch))
                         !=fh.nchunks*sizeof(struct ch))
            goto end;
        for(i=0; i<fh.nchunks; i++)
        {
            if (chs[i].pos>=25*80 || chs[i].len+chs[i].pos>25*80)
                goto end;	/* corrupted file */
            gzread(g, &scr[0][0]+chs[i].pos, chs[i].len*sizeof(attrchar));
        }
        memcpy(&screens[fh.dscr], &scr, sizeof(screen));
        
        if (note)
        {
            if (gzread(g, &nh, sizeof(nh)!=sizeof(nh)))
                goto end;
            for(i=0; i<nh.h; i++)
                gzread(g, &scr[nh.y+i][nh.x], nh.w*sizeof(attrchar));
            if (nh.titlelen>80)
                goto end;
            *bp++='\e', *bp++=']', *bp++='0', *bp++=';';
            gzread(g, bp, nh.titlelen);
            bp+=nh.titlelen;
            *bp++=7;
        }
        
        bp+=scrdiff(&tty, &scr, bp);
        
        tv.tv_sec=fh.delay/1000;
        tv.tv_usec=(fh.delay-tv.tv_sec*1000)*1000;
        synch_wait(&tv, arg);
        synch_print(buf, bp-buf, arg);
        
        bp=buf;
    }
    
end:
    gzclose(g);
}
#endif
