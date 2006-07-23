#ifdef HAVE_LANGINFO_H
#include <locale.h>
#include <langinfo.h>

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

void error(const char *txt, ...)
{
    va_list ap;

    va_start(ap, txt);
    vfprintf(stderr, txt, ap);
    exit(1);
    va_end(ap); /* make ugly compilers happy */
}
