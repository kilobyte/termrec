#include "config.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include "compat.h"
#include "error.h"
#include "threads.h"
#include "ttyrec.h"
#include "common/common.h"
#include "gettext.h"

#define BUFFER_SIZE 4096

struct addrinfo *ai;
int verbose, raw;
int lport, rport;
char *host;
char *record_name;
char *format, *format_ext;

#define EOR 239     /* End of Record */
#define SE  240     /* subnegotiation end */
#define NOP 241     /* no operation */
#define DM  242     /* Data Mark */
#define BRK 243     /* Break */
#define IP  244     /* interrupt process */
#define AO  245     /* abort output */
#define AYT 246     /* Are you there? */
#define EC  247     /* erase character */
#define EL  248     /* erase line */
#define GA  249     /* go ahead */
#define SB  250     /* subnegotiations */
#define WILL    251
#define WONT    252
#define DO      253
#define DONT    254
#define IAC 255     /* interpret as command */

/*
#define ECHO                1
#define SUPPRESS_GO_AHEAD   3
#define STATUS              5
#define TERMINAL_TYPE       24
#define END_OF_RECORD       25
#define NAWS                31

#define IS      0
#define SEND    1
*/

struct workstate
{
    int fd[2];
    volatile int echoing[2];
    recorder rec;
    void *rst;
    mutex_t mutex;
    int who;
};

void workthread(struct workstate *ws)
{
    int state,will=0 /*lint food*/,who;
    size_t cnt;
    unsigned char buf[BUFFER_SIZE],out[BUFFER_SIZE],*bp,*op;
    struct timeval tv;

    mutex_lock(ws->mutex);
    who=ws->who++;
    mutex_unlock(ws->mutex);
    state=0;
    while((cnt=recv(ws->fd[who], buf, BUFFER_SIZE, 0))>0)
    {
        if (send(ws->fd[1-who], buf, cnt, 0)<cnt)
            break;
        bp=buf;
        op=out;
#ifdef COLORCODING
        op+=sprintf(out, "\e[3%d;1m", who+1);
#endif
        if (raw)
        {
            while(cnt--)
                *op++=*bp++;
            goto no_traffic_analysis;
        }
        
        while(cnt--)
            switch(state)
            {
            default: /* normal */
                switch(*bp)
                {
                case IAC:	/* IAC = start a TELNET sequence */
                    bp++;
                    state=1;
                    break;
                default:
                    *op++=*bp++;
                }
                break;
            case 1: /* IAC */
                switch(*bp)
                {
                case IAC: 	/* IAC IAC = literal 255 byte */
                    bp++;
                    *op++=255;
                    state=0;
                    break;
                case WILL:
                case WONT:
                case DO:
                case DONT:	/* IAC WILL/WONT/DO/DONT x = negotiating option x */
                    will=*bp;
                    bp++;
                    state=2;
                    break;
                case SB:	/* IAC SB x = subnegotiations of option x */
                    bp++;
                    state=3;
                    break;
                default:
                    bp++;
                    state=0;
                }
                break;
            case 2: /* IAC WILL/WONT/DO/DONT */
                if (*bp==1)	/* ECHO */
                {
                    if (will==WILL)
                        ws->echoing[1-who]=1;
                    else if (will==WONT)
                        ws->echoing[1-who]=0;
                    /* We don't try to cope with the erroneous situation when
                     * _both_ sides claim to be echoing.  RFC857 specifically
                     * forbids this.
                     */
                }
                bp++;
                state=0;
                break;
            case 3: /* IAC SB */
                bp++;
                state=4;
                break;
            case 4: /* IAC SB x ... */
                if (*bp++==IAC)
                    state=5;
                else
                    state=4;
                /* CAVEAT: There is _no_ protection against broken IAC SB
                 *   without corresponding IAC SE.  If they're breaking the
                 *   protocol, then well, it's their fault.  We do what we
                 *   are expected to do.
                 */
                break;
            case 5: /* IAC SB x ... IAC */
                if (*bp++==SE)
                    state=0;
                else
                    state=4;
            }
        if (!ws->echoing[who])
        {
    no_traffic_analysis:
            gettimeofday(&tv, 0);
            mutex_lock(ws->mutex);
            ttyrec_w_write(ws->rec, &tv, (char*)out, op-out);
            mutex_unlock(ws->mutex);
        }
        /* FIXME: No error handling. */
    }
    shutdown(ws->fd[who], SHUT_RD);
    shutdown(ws->fd[1-who], SHUT_WR);
}


static int connect_out()
{
    struct addrinfo *addr;
    int sock;
    
    for (addr=ai; addr; addr=addr->ai_next)
    {
        if ((sock=socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol))==-1)
        {
            if (verbose)
                printf("* %s\n", strerror(errno));
            continue;
        }
        
    intr:
        if ((connect(sock, addr->ai_addr, addr->ai_addrlen)))
        {
            switch(errno)
            {
            case EINTR:
                goto intr;
            default:
                if (verbose)
                    printf("* %s\n", strerror(errno));
                continue;
            }
        }
        return sock;
    }
    return -1;
}


void connthread(void *arg)
{
    thread_t th;
    char *filename;
    struct timeval tv;
    struct workstate ws;
    int fd;
    int sock=(intptr_t)arg;
    
    if (verbose)
        printf(_("Incoming connection!\n"));
    memset(&ws, 0, sizeof(ws));
    mutex_init(ws.mutex);
    ws.fd[0]=sock;
    if ((ws.fd[1]=connect_out())<0)
    {
        closesocket(sock);
        return;
    }
    if (verbose)
        printf(_("Ok.\n")); /* managed to connect out */
    filename=record_name;
    fd=open_out(&filename, format_ext);
    gettimeofday(&tv, 0);
    if (fd==-1 || !(ws.rec=ttyrec_w_open(fd, format, filename, &tv)))
    {
        fprintf(stderr, _("Can't create file: %s\n"), filename);
        closesocket(ws.fd[0]);
        closesocket(ws.fd[1]);
        if (!record_name)
            free(filename);
        return;
    }
    if (verbose)
        printf(_("Logging to %s.\n"), filename);
    if (!record_name)
        free(filename);
    if (thread_create_joinable(&th, workthread, &ws))
    {
        fprintf(stderr, _("Can't create thread: %s\n"), strerror(errno));
        closesocket(ws.fd[0]);
        closesocket(ws.fd[1]);
        ttyrec_w_close(ws.rec);
        return;
    }
    workthread(&ws);
    thread_join(th);
    closesocket(ws.fd[0]);
    closesocket(ws.fd[1]);
    ttyrec_w_close(ws.rec);
    if (verbose)
        printf(_("Connection closed.\n"));
    mutex_destroy(ws.mutex);
}


void resolve_out()
{
    int err;
    struct addrinfo hints;
    char port[10];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;
    hints.ai_flags=AI_ADDRCONFIG|AI_NUMERICSERV;
    assert(rport>0 && rport<65536);
    sprintf(port, "%u", rport);
    
    if ((err=getaddrinfo(host, port, &hints, &ai)))
    {
        if (err==EAI_NONAME)
            die(_("No such host: %s\n"), host);
        else
            die("%s", gai_strerror(err));
    }
}


#if 0
/* postponed until listening on non-localhost gets added */
int listen_lo()
{
    int sock;
    int opt, err;
    struct addrinfo hints, *ai;
    char port[10];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;
    hints.ai_flags=AI_ADDRCONFIG|AI_NUMERICSERV;
    assert(lport>0 && lport<65536);
    sprintf(port, "%u", lport);
    
    if ((err=getaddrinfo(0, port, &hints, &ai)))
        die("%s", gai_strerror(err));
    
    if ((sock=socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol))==-1)
        die(_("Can't listen: %s\n"), strerror(errno));
    
    opt=1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    if (bind(sock, ai->ai_addr, ai->ai_addrlen))
        die(_("bind() failed: %s\n"), strerror(errno));
    if (listen(sock, 2))
        die(_("listen() failed: %s\n"), strerror(errno));
    return sock;
}
#else
int listen_lo()
{
    int sock;
    struct sockaddr_in sin;
    int opt;

    if ((sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
        die(_("socket() failed: %s\n"), strerror(errno));
    
    opt=1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr=inet_addr("127.0.0.1");
    sin.sin_port=htons(lport);
    if (bind(sock, (struct sockaddr*)&sin, sizeof(sin)))
        die(_("bind() failed: %s\n"), strerror(errno));
    if (listen(sock, 2))
        die(_("listen() failed: %s\n"), strerror(errno));
    return sock;
}
#endif


/* free the hostname from IPv6-style trappings: [::1] -> ::1, extract :rport */
void get_host_rport()
{
    long i;
    char *cp;
    
    if (*host=='[')
    {
        host++;
        cp=strchr(host, ']');
        if (!cp)
            die(_("Unmatched [ in the host part.\n"));
        *cp++=0;
        if (*cp)
        {
            if (*cp==':')
                goto getrport;
            else
                die(_("Cruft after the [host name].\n")); /* IPv6-style host name */
        }
    }
    if ((cp=strrchr(host, ':')))
    {
    getrport:
        *cp=0;
        cp++;
        i=strtoul(cp, &cp, 10);
        if (*cp || !i || i>65535)
            die(_("Invalid remote port in the host part.\n"));
        if (rport!=-1)
            die(_("You can specify the remote port at most once.\n"));
        rport=i;
    }
}


extern char *optarg;
extern int optopt;

#if (defined HAVE_GETOPT_LONG) && (defined HAVE_GETOPT_H)
static struct option proxy_opts[]={
{"format",	1, 0, 'f'},
{"local-port",	1, 0, 'l'},
{"listen-port",	1, 0, 'l'},
{"port",        1, 0, 'p'},
{"raw",		0, 0, 'r'},
{"telnet",	0, 0, 't'},
{"help",	0, 0, 'h'},
{0,		0, 0, 0},
};
#endif

void get_proxy_parms(int argc, char **argv)
{
    char *cp;
    long i;

    format=0;
    host=0;
    record_name=0;
    lport=-1;
    rport=-1;
    raw=1;
    
    while(1)
    {
#if (defined HAVE_GETOPT_LONG) && (defined HAVE_GETOPT_H)
        switch(getopt_long(argc, argv, "f:l:rthp:", proxy_opts, 0))
#else
        switch(getopt(argc, argv, "f:l:rthp:"))
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
        case 'l':
            if (lport!=-1)
                die(_("You can specify -l only once.\n"));
            i=strtoul(optarg, &cp, 10);
            if (*cp || !i || i>65535)
                die(_("Invalid local port.\n"));
            lport=i;
            break;
        case 'p':
            if (rport!=-1)
                die(_("You can specify the remote port at most once.\n"));
            i=strtoul(optarg, &cp, 10);
            if (*cp || !i || i>65535)
                die(_("Invalid remote port.\n"));
            rport=i;
            break;
        case 'r':
            raw=1;
            break;
        case 't':
            raw=0;
            break;
        case 'h':
            printf(
                "%sproxyrec [-f format] [-p rport] [-l lport] host[:port] [file]\n"
                "    %s"
                "-f, --format X        %s\n"
                "-l, --local-port X    %s\n"
                "-p, --port X          %s\n"
                "-t, --telnet          %s\n"
                "-h, --help            %s\n"
                "%s%s%s%s",
                _("Usage:"),
                _("Sets up a proxy, recording all TELNET traffic to a file, including timing data.\n"),
                _("set output format to X (-f whatever for the list)"),
                _("listen on port X locally (default: 9999)"),
                _("connect to remote port X (default: 23)"),
                _("weed out TELNET negotiations and extract some data from them"),
                _("show this usage message"),
                _("The host to connect to must be specified.\n"),
                _("If no filename is given, a name will be generated using the current date\n"
                  "    and the given format; the proxy will also allow multiple connections.\n"),
                _("If no format is given, it will be set according to the extension of the\n"
                  "    filename, or default to ttyrec if nothing is given.\n"),
                _("You can specify compression by appending .gz or .bz2 to the file name\n"));
            exit(0);
        }
    }
finish_args:
    if (optind<argc)
        host=argv[optind++];
    else
        die(_("You MUST specify the host to connect to!\n"));
    get_host_rport();
    
    if (optind<argc)
        record_name=argv[optind++];
    if (optind<argc)
        die(_("You can specify at most one file to record to.\n"));
    
    if (!format)
        format=ttyrec_w_find_format(0, record_name, "ansi");
    if (!(format_ext=ttyrec_w_get_format_ext(format)))
        format_ext="";
}


int main(int argc, char **argv)
{
    int sock,s;
    thread_t th;
    
    get_proxy_parms(argc, argv);
    if (rport==-1)
        rport=23;
    if (lport==-1)
        lport=9999;

    verbose=isatty(1);
    
    if (verbose)
        printf(_("Resolving %s...\n"), host);
    resolve_out();

    sock=listen_lo();
    if (verbose)
        printf(_("Listening...\n"));
    while((s=accept(sock, 0, 0))>=0)
    {
        if (record_name)
        {
            connthread((void*)(intptr_t)s);
            exit(0);
        }
        thread_create_detached(&th, connthread, (intptr_t)s);
    }
    return 0;
}
