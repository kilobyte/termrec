#include "config.h"
#include "string.h"
#ifdef HAVE_LANGINFO_H 
#include <locale.h> 
#include <langinfo.h>
#endif

#ifdef HAVE_LANGINFO_H 
int is_utf8()
{
    setlocale(LC_CTYPE, "");
    return !strcmp(nl_langinfo(CODESET), "UTF-8");
}
#else 
int is_utf8()
{
    return 0; /* unlikely on musty pieces of cruft */
}
#endif 
