#include "config.h"
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
#include <errno.h>
#include <assert.h>
#include "utils.h"
#include "threads.h"
#include "name_out.h"
#include "stream.h"
#include "formats.h"

#define BUFFER_SIZE 4096

#ifdef HAVE_GETADDRINFO
# define IPV6
#endif

#ifdef IPV6
struct addrinfo *ai;
#else
struct hostent *hp;
#endif
int verbose;

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
    FILE *f;
    void *rst;
    mutex_t mutex;
    int who;
};

void workthread(struct workstate *ws)
{
    int state,will,who;
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
            gettimeofday(&tv, 0);
            mutex_lock(ws->mutex);
            rec[codec].write(ws->f, ws->rst, &tv, (char*)out, op-out);
            mutex_unlock(ws->mutex);
        }
        /* FIXME: No error handling. */
    }
    shutdown(ws->fd[who], SHUT_RD);
    shutdown(ws->fd[1-who], SHUT_WR);
}


#ifdef IPV6
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
        
        freeaddrinfo(ai);
        return sock;
    }
    
    freeaddrinfo(ai);
    return -1;
}
#else
static int connect_out()
{
    struct sockaddr_in sin;
    int sock;
    
    if ((sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
    {
        fprintf(stderr, "socket() failed.\n");
        return -1;
    }
    sin.sin_family=AF_INET;
    memcpy((char *)&sin.sin_addr, hp->h_addr, sizeof(sin.sin_addr));
    sin.sin_port=htons(rport);
    if (verbose)
        printf("Connecting out...\n");
    if (connect(sock, (struct sockaddr*)&sin, sizeof(sin)))
    {
        fprintf(stderr, "connect() failed.\n");
        closesocket(sock);
        return -1;
    }
    return sock;
}
#endif


void connthread(int sock)
{
    thread_t th;
    char *filename;
    struct timeval tv;
    struct workstate ws;
    
    if (verbose)
        printf("Incoming connection!\n");
    memset(&ws, 0, sizeof(ws));
    mutex_init(ws.mutex);
    ws.fd[0]=sock;
    if ((ws.fd[1]=connect_out())<0)
    {
        closesocket(sock);
        return;
    }
    if (verbose)
        printf("Ok.\n");
    filename=record_name;
    ws.f=fopen_out(&filename, !!record_name);
    if (!ws.f)
    {
        fprintf(stderr, "Can't create logfile.\n");
        closesocket(ws.fd[0]);
        closesocket(ws.fd[1]);
        return;
    }
    if (verbose)
        printf("Logging to %s.\n", filename);
    gettimeofday(&tv, 0);
    ws.rst=rec[codec].init(ws.f, &tv);
    if (thread_create_joinable(th,workthread,&ws))
    {
        fprintf(stderr, "Can't create thread.\n");
        closesocket(ws.fd[0]);
        closesocket(ws.fd[1]);
        fclose(ws.f);
        return;
    }
    workthread(&ws);
    thread_join(th);
    closesocket(ws.fd[0]);
    closesocket(ws.fd[1]);
    rec[codec].finish(ws.f, ws.rst);
    fclose(ws.f);
    if (verbose)
        printf("Connection closed.\n");
    mutex_destroy(ws.mutex);
}


#ifdef IPV6
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
    
    if ((err=getaddrinfo(command, port, &hints, &ai)))
    {
        if (err==EAI_NONAME)
            error("No such host: %s\n", command);
        else
            error("%s", gai_strerror(err));
    }
}
#else
void resolve_out()
{
    if (!(hp=gethostbyname(command)))
        error("No such host: %s\n", command);
}
#endif


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
        error("%s", gai_strerror(err));
    
    if ((sock=socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol))==-1)
    	error("Can't listen: %s\n", strerror(errno));
    
    opt=1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    if (bind(sock, ai->ai_addr, ai->ai_addrlen))
        error("bind() failed.\n");
    if (listen(sock, 2))
        error("listen() failed.\n");
    return sock;
}
#else
int listen_lo()
{
    int sock;
    struct sockaddr_in sin;
    int opt;

    if ((sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
        error("socket() failed.\n");
    
    opt=1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr=inet_addr("127.0.0.1");
    sin.sin_port=htons(lport);
    if (bind(sock, (struct sockaddr*)&sin, sizeof(sin)))
        error("bind() failed.\n");
    if (listen(sock, 2))
        error("listen() failed.\n");
    return sock;
}
#endif


int main(int argc, char **argv)
{
    int sock,s;
    thread_t th;
    
    sockets_init();	/* Win32 only */

    get_parms(argc, argv, 1);
    if (rport==-1)
        rport=23;
    if (lport==-1)
        lport=9999;

    verbose=isatty(1);
    
    if (verbose)
        printf("Resolving %s...\n", command);
    resolve_out();

    sock=listen_lo();
    if (verbose)
        printf("Listening...\n");
    while((s=accept(sock, 0, 0))>=0)
    {
        if (record_name)
        {
            connthread(s);
#ifdef IS_WIN32
            reap_threads();
#endif
            exit(0);
        }
        thread_create_detached(th, connthread, s);
    }
    return 0;
}
