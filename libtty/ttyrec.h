#ifndef _TTYREC_H
#define _TTYREC_H
#include <sys/time.h>
#include <vt100.h>

int		open_stream(int fd, char* url, int mode);

typedef struct {} *recorder;
char*		ttyrec_w_find_format(char *format, char *filename);
recorder	ttyrec_w_open(int fd, char *format, char *filename, struct timeval *ts);
int		ttyrec_w_write(recorder r, struct timeval *delay, char *data, int len);
int		ttyrec_w_close(recorder r);
char*		ttyrec_w_get_format_name(int i);
char*		ttyrec_w_get_format_ext(int i);

char*		ttyrec_r_find_format(char *format, char *filename);
char*		ttyrec_r_get_format_name(int i);
char*		ttyrec_r_get_format_ext(int i);
int		ttyrec_r_play(int fd, char *format, char *filename,
		    void (*synch_init_wait)(struct timeval *ts, void *arg),
		    void (*synch_wait)(struct timeval *tv, void *arg),
		    void (*synch_print)(char *buf, int len, void *arg),
		    void *arg);

typedef struct {} *ttyrec;
typedef struct
{
    struct timeval t;
    int len;
    char *data;
} *ttyrec_frame;

ttyrec		ttyrec_init(vt100 vt);
ttyrec		ttyrec_load(int fd, char *format, char *filename, vt100 vt);
void		ttyrec_free(ttyrec tr);
ttyrec_frame	ttyrec_seek(ttyrec tr, struct timeval *t, vt100 *vt);
ttyrec_frame	ttyrec_next_frame(ttyrec tr, ttyrec_frame tfv);
void		ttyrec_add_frame(ttyrec tr, struct timeval *delay, char *data, int len);
int		ttyrec_save(ttyrec tr, int fd, char *format, char *filename,
                    struct timeval *selstart, struct timeval *selend);
#endif
