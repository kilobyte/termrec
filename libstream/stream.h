export int open_stream(int fd, const char* url, int mode, const char **error);
int open_file(const char* url, int mode, const char **error);
int open_tcp(const char* url, int mode, const char **error);
int open_telnet(const char* url, int mode, const char **error);
int open_termcast(const char* url, int mode, const char **error);
int match_suffix(const char *txt, const char *ext, int skip);
int match_prefix(const char *txt, const char *ext);
int filter(void func(int,int,const char*), int fd, int wr, const char *arg, const char **error);

int connect_tcp(const char *url, int port, const char **rest, const char **error);
void telnet(int sock, int fd, const char *arg);

// bits: 1 = writing, 2 = growing
#define SM_READ         0
#define SM_WRITE        1
#define SM_REPREAD      2
#define SM_APPEND       3
