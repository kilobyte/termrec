typedef void(compress_func)(int,int,char*);

typedef struct
{
    char                *name;
    char                *ext;
    compress_func       *comp;
} compress_info;

extern compress_info compressors[];
extern compress_info decompressors[];

compress_info *comp_from_ext(char *name, compress_info *ci);
