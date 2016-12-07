int open_curl(const char* url, int mode, const char **error);
int open_file(const char* url, int mode, const char **error);
int open_tcp(const char* url, int mode, const char **error);
int open_unix(const char* socket, int mode, const char **error);
int open_telnet(const char* url, int mode, const char **error);
int open_termcast(const char* url, int mode, const char **error);
int match_suffix(const char *txt, const char *ext, int skip);
int match_prefix(const char *txt, const char *ext);
int filter(void func(int,int,const char*), int fd, int wr, const char *arg, const char **error);

int connect_tcp(const char *url, int port, const char **rest, const char **error);
int connect_unix(const char *socket_path, const char **error);
void telnet(int sock, int fd, const char *arg);
#ifdef IS_WIN32
void reap_streams(void);
#endif
