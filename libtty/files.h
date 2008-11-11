export int ttyrec_r_play(int fd, char *format, char *filename,
    void (*synch_init_wait)(struct timeval *ts, void *arg),
    void (*synch_wait)(struct timeval *tv, void *arg),
    void (*synch_print)(char *buf, int len, void *arg),
    void *arg);
typedef struct {} *recorder;
export recorder	ttyrec_w_open(int fd, char *format, char *filename, struct timeval *ts);
export int	ttyrec_w_write(recorder r, struct timeval *delay, char *data, int len);
export int	ttyrec_w_close(recorder r);

#define tcmp(t1, t2)	(((t1).tv_sec>(t2).tv_sec)?1:		\
                         ((t1).tv_sec<(t2).tv_sec)?-1:		\
                         ((t1).tv_usec>(t2).tv_usec)?1:		\
                         ((t1).tv_usec<(t2).tv_usec)?-1:0)
#define tadd(t, d)	{if (((t).tv_usec+=(d).tv_usec)>1000000)	\
                            (t).tv_usec-=1000000, (t).tv_sec++;		\
                         (t).tv_sec+=(d).tv_sec;			\
                        }
