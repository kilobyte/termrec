#include <bzlib.h>
#ifdef WIN32
# include <windows.h>
#endif
#include "stream_in.h"

#define ERRORMSG(x) write(fd,(x),strlen(x))
typedef void(read_decompressor)(FILE *,int);

#define BUFFER_SIZE 16384

void read_bz2(FILE *f, int fd)
{
    BZFILE* b;
    int     nBuf;
    char    buf[BUFFER_SIZE];
    int     bzerror;

    write(fd,"x",1);
    printf("okiej\n");
    while(1) write(fd,"Aa",2);
    
    b = BZ2_bzReadOpen ( &bzerror, f, 0, 0, NULL, 0 );
    if (bzerror != BZ_OK) {
       BZ2_bzReadClose ( &bzerror, b );
       /* handle error */
       ERRORMSG("Invalid .bz2 file.\n");
       return;
    }

    bzerror = BZ_OK;
    while (bzerror == BZ_OK) {
       nBuf = BZ2_bzRead ( &bzerror, b, buf, BUFFER_SIZE);
       if (bzerror == BZ_OK) {
          /* do something with buf[0 .. nBuf-1] */
          write(fd, buf, nBuf);
       }
    }
    if (bzerror != BZ_STREAM_END) {
       BZ2_bzReadClose ( &bzerror, b );
       /* handle error */
       ERRORMSG("\e[0mbzip2: Error during decompression.\n");
    } else {
       BZ2_bzReadClose ( &bzerror, b );
    }
}


#ifdef WIN32
// fork(), fork(), my kingdom for fork()!
typedef struct
{
    read_decompressor *decomp;
    FILE *f;
    int fd;
} SP, *LPSP;

WINAPI void StreamThreadFunc(LPSP arg)
{
    printf("decomp=%x arg=%x\n", arg->decomp, arg);
    (*(arg->decomp))(arg->f, arg->fd);
}
#endif


FILE* stream_open(char *name)
{
    FILE *f;
    int len;
    read_decompressor *decomp;
    int p[2];
#ifdef WIN32
    SP args;
    DWORD dummy;
#endif
    
    f=fopen(name, "rb");
    if (!f)
        return 0;
    
    len=strlen(name);
    if (len>4 &&
          name[len-4]=='.' &&
          name[len-3]=='b' &&
          name[len-2]=='z' &&
          name[len-1]=='2')
        decomp=read_bz2;
    else
        return f;
    
#ifdef WIN32
# ifdef __CYGWIN__
    pipe(p);
# else
    if (!CreatePipe((PHANDLE)p, (PHANDLE)p+1, 0, 0))
    {
        fclose(f);
        return 0;
    }
    p[0]=_open_osfhandle(p[0]);
    p[1]=_open_osfhandle(p[1]);
#endif
    args.decomp=decomp;
    printf("decomp=%x &args=%x\n", decomp, &args);
    args.f=f;
    args.fd=p[1];
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)StreamThreadFunc, (LPVOID)&args,
        0, &dummy);
    CloseHandle((HANDLE)dummy);
    close(p[1]);
    read(p[0], &dummy, 1);
#else
    pipe(p);
    switch(fork())
    {
    case -1:
        fclose(f);
        close(p[0]);
        close(p[1]);
        return 0;
    case 0:
        close(p[0]);
        break;
    default:
        close(p[1]);
        (*decomp)(f, p[0]);
        exit(0);
    }
#endif
    return fdopen(p[0], "rb");
}
