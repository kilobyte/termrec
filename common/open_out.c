#include "config.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "error.h"
#include "compat.h"
#include "ttyrec.h"
#include "gettext.h"


#if (defined HAVE_LIBBZ2) || (defined SHIPPED_LIBBZ2)
#define comp_ext ".bz2"
#else
# if (defined HAVE_LIBZ) || (defined SHIPPED_LIBZ)
#define comp_ext ".gz"
# else
#define comp_ext ""
# endif
#endif


/* Generate the next name in the sequence: "", a, b, ... z, aa, ab, ... */
static void nameinc(char *add)
{
    char *ae,*ai;

    ae=add;
    while(*ae)
        ae++;
    ai=ae;      /* start at the end of the string */
    while(1)
    {
        if (--ai<add)
        {
            *ae++='a';  /* if all combinations are exhausted, */
            *ae=0;      /*  append a new letter */
            return;
        }
        if (*ai!='z')
        {
            (*ai)++;	/* increase the first non-'z' */
            return;
        }
        *ai='a';	/* ... replacing 'z's by 'a' */
    }
}


int open_out(char **file_name, char *format_ext)
{
    int fd;
    char add[10],date[24];
    time_t t;
    
    if (!*file_name)
    {
        *add=0;
        time(&t);
        strftime(date, sizeof(date), "%Y-%m-%d.%H-%M-%S", localtime(&t));
        while(1)
        {
            asprintf(file_name, "%s%s%s%s", date, add, format_ext, comp_ext);
            fd=open(*file_name, O_CREAT|O_WRONLY|O_EXCL|O_BINARY, 0666);
            /* We do some autoconf magic to exclude O_BINARY when inappropiate. */
            if (fd!=-1)
                goto finish;
            if (errno!=EEXIST)
                break;
            free(*file_name);
            *file_name=0;
            nameinc(add);
        }
        die(_("Can't create a valid file in the current directory: %s\n"), strerror(errno));
        return -1;
    }
    if (!strcmp(*file_name, "-"))
    {
        fd=-1;
        goto finish;
    }
    if (!(fd=open(*file_name, O_WRONLY|O_CREAT, 0666)))
        die(_("Can't write to the record file (%s): %s\n"), *file_name, strerror(errno));
finish:
    return open_stream(fd, *file_name, O_WRONLY|O_CREAT);
}
