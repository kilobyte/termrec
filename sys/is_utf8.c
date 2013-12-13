#include "config.h"
#include <string.h>
#ifdef HAVE_LANGINFO_H
#include <locale.h>
#include <langinfo.h>
#endif
#include "export.h"

#if defined(HAVE_LANGINFO_H) && !defined(IS_WIN32)
static int initialized=0;

export int is_utf8()
{
    if (!initialized)
    {
        initialized=1;
        // if locale looks already initialized, don't mess with it
        if (!strcmp(setlocale(LC_CTYPE,0), "C"))
            // we don't know if they want to use something else...
            setlocale(LC_CTYPE, "");
    }
    return !strcmp(nl_langinfo(CODESET), "UTF-8");
}
#else
export int is_utf8()
{
    return 0; // unlikely on musty pieces of cruft
}
#endif
