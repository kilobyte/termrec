export int open_stream(int fd, char* url, int mode);
int match_suffix(char *txt, char *ext, int skip);
int match_prefix(char *txt, char *ext);

/* bits: 1 = writing, 2 = growing */
#define M_READ		0
#define M_WRITE		1
#define M_REPREAD	2
#define M_APPEND	3
