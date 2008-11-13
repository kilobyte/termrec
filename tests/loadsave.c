#include <ttyrec.h>
#include "config.h"
#include "sys/error.h"

int main(int argc, char **argv)
{
    ttyrec tr;
    
    if (argc!=3)
        error("Usage: loadsave f1 f2\n");
    
    if (!(tr=ttyrec_load(-1, 0, argv[1], 0)))
        error("Can't read the ttyrec from %s\n", argv[1]);
    if (!ttyrec_save(tr, -1, 0, argv[2], 0, 0))
        error("Can't write the ttyrec to %s\n", argv[2]);
    ttyrec_free(tr);
    return 0;
}
