#include <stdio.h>

typedef void play_func(FILE *f);
typedef void *(record_func_init)(FILE *f, struct timeval *tm);
typedef void (record_func)(FILE *f, void* state, struct timeval *tm, char *buf, int len);
typedef void (record_func_finish)(FILE *f, void* state);

typedef struct
{
    char		*name;
    char		*ext;
    record_func_init	*init;
    record_func		*write;
    record_func_finish	*finish;
} recorder_info;

typedef struct
{
    char		*name;
    char		*ext;
    play_func		*play;
} player_info;

extern recorder_info rec[];
extern player_info play[];
