#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "sys/error.h"
#include "gettext.h"
#include <ttyrec.h>

extern char *optarg;

void get_w_format(char **format)
{
    int i;
    char *fn, *fe;
    
    if (*format)
        error(_("You can use only one format at a time.\n"));
    if (!(*format=ttyrec_w_find_format(optarg, 0, 0)))
    {
        fprintf(stderr, _("No such format: %s\n"), optarg);
        fprintf(stderr, _("Valid formats:\n"));
        for(i=0;(fn=ttyrec_w_get_format_name(i));i++)
            if ((fe=ttyrec_w_get_format_ext(fn)))
                fprintf(stderr, " %-15s (%s)\n", fn, fe);
            else
                fprintf(stderr, " %-15s\n", fn);
        exit(1);
    }
}
