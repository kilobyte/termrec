#ifndef _TTYREC_H
#define _TTYREC_H
#include <sys/time.h>
#include <vt100.h>

#ifndef _TTYREC_H_NO_TYPES
typedef struct {} *recorder;
typedef struct {} *ttyrec;
typedef struct
{
    struct timeval t;
    int len;
    char *data;
} *ttyrec_frame;
#endif

#define M_READ		0
#define M_WRITE		1
#define M_REPREAD	2
#define M_APPEND	3

int		open_stream(int fd, char* url, int mode, char **error);

char*		ttyrec_w_find_format(char *format, char *filename, char *fallback);
recorder	ttyrec_w_open(int fd, char *format, char *filename, struct timeval *ts);
int		ttyrec_w_write(recorder r, struct timeval *tm, char *data, int len);
int		ttyrec_w_close(recorder r);
char*		ttyrec_w_get_format_name(int i);
char*		ttyrec_w_get_format_ext(char *format);

char*		ttyrec_r_find_format(char *format, char *filename, char *fallback);
char*		ttyrec_r_get_format_name(int i);
char*		ttyrec_r_get_format_ext(char *format);
int		ttyrec_r_play(int fd, char *format, char *filename,
		    void (*synch_init_wait)(struct timeval *ts, void *arg),
		    void (*synch_wait)(struct timeval *delay, void *arg),
		    void (*synch_print)(char *data, int len, void *arg),
		    void *arg);

ttyrec		ttyrec_init(vt100 vt);
ttyrec		ttyrec_load(int fd, char *format, char *filename, vt100 vt);
void		ttyrec_free(ttyrec tr);
ttyrec_frame	ttyrec_seek(ttyrec tr, struct timeval *t, vt100 *vt);
ttyrec_frame	ttyrec_next_frame(ttyrec tr, ttyrec_frame tfv);
void		ttyrec_add_frame(ttyrec tr, struct timeval *delay, char *data, int len);
int		ttyrec_save(ttyrec tr, int fd, char *format, char *filename,
                    struct timeval *selstart, struct timeval *selend);


#define tadd(t, d)	{if (((t).tv_usec+=(d).tv_usec)>=1000000)	\
                            (t).tv_usec-=1000000, (t).tv_sec++;		\
                         (t).tv_sec+=(d).tv_sec;			\
                        }
#define tsub(t, d)	{if ((signed)((t).tv_usec-=(d).tv_usec)<0)	\
                            (t).tv_usec+=1000000, (t).tv_sec--;		\
                         (t).tv_sec-=(d).tv_sec;			\
                        }
#define tmul1000(t, m)	{long long v;					\
                         v=((long long)(t).tv_usec)*(m)/1000+		\
                             ((long long)(t).tv_sec)*(m)*1000;		\
                         (t).tv_usec=v%1000000;				\
                         (t).tv_sec=v/1000000;				\
                         if ((t).tv_usec<0)				\
                             (t).tv_usec+=1000000, (t).tv_sec--;	\
                        }
#define tdiv1000(t, m)	{long long v;					\
                         v=((long long)(t).tv_sec)*1000000+(t).tv_usec;	\
                         v*=1000;					\
                         v/=m;						\
                         (t).tv_usec=v%1000000;				\
                         (t).tv_sec=v/1000000;				\
                         if ((t).tv_usec<0)				\
                             (t).tv_usec+=1000000, (t).tv_sec--;	\
                        }
#define tcmp(t1, t2)	(((t1).tv_sec>(t2).tv_sec)?1:		\
                         ((t1).tv_sec<(t2).tv_sec)?-1:		\
                         ((t1).tv_usec>(t2).tv_usec)?1:		\
                         ((t1).tv_usec<(t2).tv_usec)?-1:0)
#endif
