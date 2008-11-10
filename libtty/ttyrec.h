int		open_stream(int fd, char* url, int mode);

typedef struct {} *recorder;
char*		ttyrec_w_find_format(char *format, char *filename);
recorder	ttyrec_w_open(int fd, char *format, char *filename, struct timeval *tm);
int		ttyrec_w_write(recorder r, struct timeval *tm, char *buf, int len);
int		ttyrec_w_close(recorder r);
char*		ttyrec_w_get_format_name(int i);
char*		ttyrec_w_get_format_ext(int i);

char*		ttyrec_r_find_format(char *format, char *filename);
char*		ttyrec_r_get_format_name(int i);
char*		ttyrec_r_get_format_ext(int i);
int		ttyrec_r_play(int fd, char *format, char *filename,
		    void *(synch_init_wait)(struct timeval *ts),
		    void *(synch_wait)(struct timeval *tv),
		    void *(synch_print)(char *buf, int len),
		    void *arg);
