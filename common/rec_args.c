#include "config.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include "error.h"
#include "compat.h"
#include "ttyrec.h"
#include "rec_args.h"
#include "gettext.h"
#include "common.h"


#ifdef HAVE_GETOPT_LONG
static struct option rec_opts[]={
{"format",      1, 0, 'f'},
{"exec",        1, 0, 'e'},
{"raw",         0, 0, 'r'},
{"append",      0, 0, 'a'},
{"help",        0, 0, 'h'},
{0,             0, 0, 0},
};
#endif

static const char *comp_ext;

void get_rec_parms(int argc, char **argv)
{
    format=0;
    command=0;
    record_name=0;
    raw=0;
    append=0;
#if (defined HAVE_LIBBZ2) || (defined SHIPPED_LIBBZ2)
    comp_ext=".bz2";
#elif (defined HAVE_LIBZ) || (defined SHIPPED_LIBZ)
    comp_ext=".gz";
#elif (defined HAVE_LIBLZMA) || (defined SHIPPED_LIBLZMA)
    comp_ext=".xz";
#else
    comp_ext="";
#endif

    while (1)
    {
#ifdef HAVE_GETOPT_LONG
        switch (getopt_long(argc, argv, "f:e:rah", rec_opts, 0))
#else
        switch (getopt(argc, argv, "f:e:rah"))
#endif
        {
        case -1:
            goto finish_args;
        case ':':
        case '?':
            exit(1);
        case 'f':
            get_w_format(&format);
            break;
        case 'e':
            if (command)
                die(_("You can specify -e only once.\n"));
            command=optarg;
            break;
        case 'r':
            raw=1;
            break;
        case 'a':
            append=1;
            break;
        case 'h':
            printf(
                "%stermrec [-f format] [-e command] [file]\n"
                "    %s"
                "-f, --format X        %s\n"
                "-e, --exec X          %s\n"
                "-r, --raw             %s\n"
                "-a, --append          %s\n"
                "-h, --help            %s\n"
                "%s%s%s",
                _("Usage: "),
                _("Records the output of a console session to a file, including timing data.\n"),
                _("set output format to X (-f whatever for the list)"),
                _("execute command X instead of spawning a shell"),
                _("don't record UTFness or terminal size"),
                _("append to an existing file"),
                _("show this usage message"),
                _("If no filename is given, a name will be generated using the current date\n"
                  "    and the given format.\n"),
                _("If no format is given, it will be set according to the extension of the\n"
                  "    filename, or default to ttyrec if nothing is given.\n"),
                _("You can specify compression by appending .gz, .xz or .bz2 to the file name.\n"));
            exit(0);
        }
    }
finish_args:
    if (optind<argc)
        record_name=argv[optind++];
    if (optind<argc)
        die(_("You can specify at most one file to record to.\n"));

    if (!format)
        format=ttyrec_w_find_format(0, record_name, "ttyrec");
    if (!(format_ext=ttyrec_w_get_format_ext(format)))
        format_ext="";
}
