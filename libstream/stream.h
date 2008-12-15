export int open_stream(int fd, char* url, int mode);
int open_file(char* url, int mode);
int open_tcp(char* url, int mode, const char **error);
int match_suffix(char *txt, char *ext, int skip);
int match_prefix(char *txt, char *ext);
int filter(void func(int,int), int fd, int wr);

/* bits: 1 = writing, 2 = growing */
#define M_READ		0
#define M_WRITE		1
#define M_REPREAD	2
#define M_APPEND	3
