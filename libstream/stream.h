export int open_stream(int fd, char* url, int mode, char **error);
int open_file(char* url, int mode, char **error);
int open_tcp(char* url, int mode, char **error);
int open_telnet(char* url, int mode, char **error);
int open_termcast(char* url, int mode, char **error);
int match_suffix(char *txt, char *ext, int skip);
int match_prefix(char *txt, char *ext);
int filter(void func(int,int,char*), int fd, int wr, char *arg, char **error);

int connect_tcp(char *url, int port, char **rest, char **error);
void telnet(int sock, int fd, char *arg);

/* bits: 1 = writing, 2 = growing */
#define M_READ		0
#define M_WRITE		1
#define M_REPREAD	2
#define M_APPEND	3
