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
#include "utils.h"
#include "error.h"
#include "compat.h"
#include "ttyrec.h"
#include "name_out.h"
#include "gettext.h"


extern char *optarg;
extern int optopt;

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


#if (defined HAVE_GETOPT_LONG) && (defined HAVE_GETOPT_H)
# define _GNU_SOURCE
# include <getopt.h>
static struct option rec_opts[]={
{"format",	1, 0, 'f'},
{"exec",	1, 0, 'e'},
{"raw",		0, 0, 'r'},
{"help",	0, 0, 'h'},
{0,		0, 0, 0},
};

static struct option proxy_opts[]={
{"format",	1, 0, 'f'},
{"local-port",	1, 0, 'l'},
{"listen-port",	1, 0, 'l'},
{"port",        1, 0, 'p'},
{"raw",		0, 0, 'r'},
{"help",	0, 0, 'h'},
{0,		0, 0, 0},
};
#endif

#define REC	0
#define PROXY	1

static char *comp_ext;

void get_parms(int argc, char **argv, int prog)
{
    char *cp;
    int i;

    format=0;
    command=0;
    record_name=0;
    lport=-1;
    rport=-1;
    raw=0;
#if (defined HAVE_LIBBZ2) || (defined SHIPPED_LIBBZ2)
    comp_ext=".bz2";
#else
# if (defined HAVE_LIBZ) || (defined SHIPPED_LIBZ)
    comp_ext=".gz";
# else
    comp_ext="";
# endif
#endif
    
    while(1)
    {
#if (defined HAVE_GETOPT_LONG) && (defined HAVE_GETOPT_H)
        switch(getopt_long(argc, argv, (prog==REC)?"f:e:rh":"f:l:rhp:",
                                       (prog==REC)?rec_opts:proxy_opts, 0))
#else
        switch(getopt(argc, argv, (prog==REC)?"f:e:rh":"f:l:rhp:"))
#endif
        {
        case -1:
            goto finish_args;
        case ':':
        case '?':
            exit(1);
        case 'f':
            if (format)
                error(_("You can use only one format at a time.\n"));
            if (!(format=ttyrec_w_find_format(optarg, 0)))
            {
                fprintf(stderr, _("No such format: %s\n"), optarg);
                fprintf(stderr, _("Valid formats:\n"));
                for(i=0;ttyrec_w_get_format_name(i);i++)
                    fprintf(stderr, " %-15s (%s)\n",
                        ttyrec_w_get_format_name(i),
                        ttyrec_w_get_format_ext(i));
                exit(1);
            }
            break;
        case 'e':
            if (command)
                error(_("You can specify -e only once.\n"));
            command=optarg;
            break;
        case 'l':
            if (lport!=-1)
                error(_("You can specify -l only once.\n"));
            i=0;
            cp=optarg;
            while(isdigit(*cp))
            {
                i=i*10+*cp++-'0';
                if (i>65535)
                    error(_("Too high local port number.\n"));
            }
            if (*cp || !i)
                error(_("Invalid local port.\n"));
            lport=i;
            break;
        case 'p':
            if (rport!=-1)
                error(_("You can specify the remote port at most once.\n"));
            i=0;
            cp=optarg;
            while(isdigit(*cp))
            {
                i=i*10+*cp++-'0';
                if (i>65535)
                    error(_("Too high remote port number.\n"));
            }
            if (*cp || !i)
                error(_("Invalid remote port.\n"));
            rport=i;
            break;
        case 'r':
            raw=1;
            break;
        case 'h':
            switch(prog)
            {
            case REC:
                printf(
                    "%stermrec [-f format] [-e command] [file]\n"
                    "    %s"
                    "-f, --format X        %s\n"
                    "-e, --exec X          %s\n"
                    "-r, --raw             %s\n"
                    "-h, --help            %s\n"
                    "%s%s%s",
                    _("Usage: "),
                    _("Records the output of a console session to a file, including timing data.\n"),
                    _("set output format to X (-f whatever for the list)"),
                    _("execute command X instead of spawning a shell"),
                    _("don't record UTFness or terminal size"),
                    _("show this usage message"),
                    _("If no filename is given, a name will be generated using the current date\n"
                      "    and the given format.\n"),
                    _("If no format is given, it will be set according to the extension of the\n"
                      "    filename, or default to ttyrec if nothing is given.\n"),
                    _("You can specify compression by appending .gz or .bz2 to the file name.\n"));
                break;
            case PROXY:
                printf(
                    "%sproxyrec [-f format] [-p rport] [-l lport] host[:port] [file]\n"
                    "    %s"
                    "-f, --format X        %s\n"
                    "-l, --local-port X    %s\n"
                    "-p, --port X          %s\n"
                    "-r, --raw             %s\n"
                    "-h, --help            %s\n"
                    "%s%s%s%s",
                    _("Usage:"),
                    _("Sets up a proxy, recording all TELNET traffic to a file, including timing data.\n"),
                    _("set output format to X (-f whatever for the list)"),
                    _("listen on port X locally (default: 9999)"),
                    _("connect to remote port X (default: 23)"),
                    _("don't weed out or parse TELNET negotiations for"),
                    _("show this usage message"),
                    _("The host to connect to must be specified.\n"),
                    _("If no filename is given, a name will be generated using the current date\n"
                      "    and the given format; the proxy will also allow multiple connections.\n"),
                    _("If no format is given, it will be set according to the extension of the\n"
                      "    filename, or default to ttyrec if nothing is given.\n"),
                    _("You can specify compression by appending .gz or .bz2 to the file name\n"));
            }
            exit(0);
        }
    }
finish_args:
    if (optind+prog>argc)
        error(_("You MUST specify the host to connect to!\n"));
    if (optind+1+prog<argc)
        error(_("You can specify at most one file to record to.\n"));
    if (prog==PROXY)
    {
        command=argv[optind];
        if (*command=='[')
        {
            command++;
            cp=strchr(command, ']');
            if (!cp)
                error(_("Unmatched [ in the host part.\n"));
            *cp++=0;
            if (*cp)
            {
                if (*cp==':')
                    goto getrport;
                else
                    error(_("Cruft after the [host name].\n")); /* IPv6-style host name */
            }
        }
        if ((cp=strrchr(command, ':')))
        {
        getrport:
            *cp=0;
            cp++;
            i=0;
            while(isdigit(*cp))
            {
                i=i*10+*cp++-'0';
                if (i>65535)
                    error(_("Too high remote port number.\n"));
            }
            if (*cp || !i)
                error(_("Invalid remote port in the host part.\n"));
            if (rport!=-1)
                error(_("You can specify the remote port at most once.\n"));
            rport=i;
        }
    }
    if (optind+prog<argc)
    {
        record_name=argv[argc-1];
        
        if (!format)
            if (!(format=ttyrec_w_find_format(0, record_name)))
                format="ttyrec";
    }
    else
        if (!format)
            format="ttyrec";
    format_ext="";
    for(i=0; (cp=ttyrec_w_get_format_name(i)); i++)
        if (!strcmp(cp, format))
            format_ext=ttyrec_w_get_format_ext(i);
}


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


int fopen_out(char **file_name, int nodetach)
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
        fprintf(stderr, _("Can't create a valid file in the current directory: %s\n"), strerror(errno));
        return -1;
    }
    if (!(fd=open(*file_name, O_WRONLY|O_CREAT, 0x666)))
        error(_("Can't write to the record file (%s): %s\n"), *file_name, strerror(errno));
finish:
    return open_stream(fd, *file_name, O_WRONLY|O_CREAT);
}
