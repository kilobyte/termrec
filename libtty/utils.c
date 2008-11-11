#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#ifdef HAVE_LANGINFO_H 
#include <locale.h> 
#include <langinfo.h> 
#endif
#include "export.h"
#include "_stdint.h"


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

#ifdef HAVE_LANGINFO_H 
export int is_utf8()
{
    setlocale(LC_CTYPE, "");
    return !strcmp(nl_langinfo(CODESET), "UTF-8");
}
#else 
export int is_utf8()
{
    return 0; /* unlikely on musty pieces of cruft */
}
#endif 
