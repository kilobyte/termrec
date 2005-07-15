#include "config.h"
#define _GNU_SOURCE
#include <string.h>
#include "utils.h"
#include "formats.h"
#include "stream.h"
#include "name.h"


int codec_from_ext_play(char *name)
{
    int i,len;

    len=0;
    for (i=0;decompressors[i].name;i++)
        if (decompressors[i].ext && match_suffix(name, decompressors[i].ext, 0))
        {
            len=strlen(decompressors[i].ext);
            break;
        }
    for (i=0;play[i].name;i++)
        if (play[i].ext && match_suffix(name, play[i].ext, len))
            return i;
    return -1;
}
