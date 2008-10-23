#include "config.h"
#define _GNU_SOURCE
#include <string.h>
#include "utils.h"
#include "formats.h"
#include "name.h"


int codec_from_ext_play(char *name)
{
    int i;

    for (i=0;play[i].name;i++)
        if (play[i].ext && match_suffix(name, play[i].ext, 0))
            return i;
    return -1;
}
