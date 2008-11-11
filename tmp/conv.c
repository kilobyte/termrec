#include <ttyrec.h>
#include "config.h"
#include "sys/error.h"

int main()
{
    ttyrec tr;
    recorder rec;
    ttyrec_frame fr;
    
    if (!(tr=ttyrec_load(-1, "ttyrec", "-", 0)))
        error("Can't read the ttyrec from stdin.");
    fr=ttyrec_seek(tr, 0, 0);
    if (!(rec=ttyrec_w_open(-1, "ttyrec", "-", &fr->t)))
        error("Can't write the ttyrec to stdout.");
    while(fr)
    {
        ttyrec_w_write(rec, &fr->t, fr->data, fr->len);
        fr=ttyrec_next_frame(tr, fr);
    }
    ttyrec_w_close(rec);
    ttyrec_free(tr);
    return 0;
}
