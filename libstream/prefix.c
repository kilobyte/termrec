#include "config.h"
#include <strings.h>
#include <string.h>
#include "export.h"

int match_suffix(char *txt, char *ext, int skip)
{
    int tl,el;

    tl=strlen(txt);
    el=strlen(ext);
    if (tl-el-skip<0)
        return 0;
    txt+=tl-el-skip;
    return (!strncasecmp(txt, ext, el));
}

int match_prefix(char *txt, char *ext)
{
    return (!strncasecmp(txt, ext, strlen(ext)));
}
