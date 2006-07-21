typedef void(compress_func)(FILE *,int);

typedef struct
{
    char                *name;
    char                *ext;
    compress_func	*comp;
} compress_info;

extern compress_info compressors[];
extern compress_info decompressors[];

FILE* stream_open(FILE *f, char *name, char *mode, compress_info *comptable, int nodetach);
void stream_reap_threads();
