#include <windows.h>

int message_loop(HANDLE* lphObjects, int cObjects);
#undef stderr
#define stderr stdout

/* CygWin may have those */
#ifndef HAVE_GETTIMEOFDAY
void gettimeofday(struct timeval *tv, void * dummy);
#endif
#ifndef HAVE_USLEEP
void usleep(unsigned int usec);
#endif
#ifndef HAVE_PIPE
int pipe(int pipefd[2]);
#endif

void* LoadFunc(char *dll, char *name);
void show_error(char *blah);
