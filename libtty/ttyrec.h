typedef struct {} *recording;
char*		ttyrec_w_find_format(char *format, char *filename);
recording	ttyrec_w_open(int fd, char *format, char *filename, struct timeval *tm);
int		ttyrec_w_write(recording r, struct timeval *tm, char *buf, int len);
int		ttyrec_w_close(recording r);
char*		ttyrec_w_get_format_name(int i);
char*		ttyrec_w_get_format_ext(int i);
