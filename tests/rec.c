#include <ttyrec.h>
#include <time.h>
#include "config.h"
#include "sys/error.h"

int main(int argc, char **argv)
{
    recorder r;
    struct timeval tv;
    
    if (argc!=2)
        error("Usage: rec outfile\n");
    
    tv.tv_sec=20;
    tv.tv_usec=10;
    if (!(r=ttyrec_w_open(-1, 0, argv[1], &tv)))
        error("Can't write the ttyrec to %s\n", argv[1]);
    tv.tv_sec=1;
    tv.tv_usec=0;
    ttyrec_w_write(r, &tv, "Abc", 3);
    ttyrec_w_write(r, &tv, "D", 1);
    ttyrec_w_write(r, &tv, "Ef", 2);
    ttyrec_w_write(r, &tv, "G", 1);
    ttyrec_w_write(r, &tv, "Hij\n", 4);
    ttyrec_w_close(r);
    return 0;
}
