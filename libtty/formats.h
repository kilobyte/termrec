#include <stdio.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

typedef void play_func(FILE *f,
    void (*synch_init_wait)(const struct timeval *ts, void *arg),
    void (*synch_wait)(const struct timeval *tv, void *arg),
    void (*synch_print)(const char *buf, int len, void *arg),
    void *arg, const struct timeval *cont);

typedef void *(record_func_init)(FILE *f, const struct timeval *tm);
typedef void (record_func)(FILE *f, void* state, const struct timeval *tm, const char *buf, int len);
typedef void (record_func_finish)(FILE *f, void* state);

typedef struct
{
    const char          *name;
    const char          *ext;
    record_func_init    *init;
    record_func         *write;
    record_func_finish  *finish;
} recorder_info;

typedef struct
{
    const char          *name;
    const char          *ext;
    play_func           *play;
} player_info;

extern recorder_info rec[];
extern player_info play[];

extern int rec_n, play_n;

void play_dosrecorder(FILE *f,
    void (*synch_init_wait)(const struct timeval *ts, void *arg),
    void (*synch_wait)(const struct timeval *tv, void *arg),
    void (*synch_print)(const char *buf, int len, void *arg),
    void *arg, const struct timeval *cont);
void play_asciicast(FILE *f,
    void (*synch_init_wait)(const struct timeval *ts, void *arg),
    void (*synch_wait)(const struct timeval *tv, void *arg),
    void (*synch_print)(const char *buf, int len, void *arg),
    void *arg, const struct timeval *cont);
