#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "export.h"
#include "stream.h"
#include "gettext.h"
#include "compat.h"



#define EOR 239     // End of Record
#define SE  240     // subnegotiation end
#define NOP 241     // no operation
#define DM  242     // Data Mark
#define BRK 243     // Break
#define IP  244     // interrupt process
#define AO  245     // abort output
#define AYT 246     // Are you there?
#define EC  247     // erase character
#define EL  248     // erase line
#define GA  249     // go ahead
#define SB  250     // subnegotiations
#define WILL    251
#define WONT    252
#define DO      253
#define DONT    254
#define IAC     255     // interpret as command

#define ECHO                1
#define SUPPRESS_GO_AHEAD   3
#define STATUS              5
#define TERMINAL_TYPE       24
#define END_OF_RECORD       25
#define NAWS                31

#define IS      0
#define SEND    1

void telnet(int sock, int fd, char *arg)
{
    int state,will=0 /*lint food*/,sublen=0;
    ssize_t cnt;
    unsigned char buf[BUFSIZ],out[BUFSIZ],*bp,*op,neg[3];

    neg[0]=IAC;
    state=0;
    while ((cnt=recv(sock, (char*)buf, BUFSIZ, 0))>0)
    {
        bp=buf;
        op=out;

        while (cnt--)
        {
            switch (state)
            {
            default: // normal
                switch (*bp)
                {
                case IAC:       // IAC = start a TELNET sequence
                    bp++;
                    state=1;
                    break;
                default:
                    *op++=*bp++;
                }
                break;
            case 1: // IAC
                switch (*bp)
                {
                case IAC:       // IAC IAC = literal 255 byte
                    bp++;
                    *op++=255;
                    state=0;
                    break;
                case WILL:
                case WONT:
                case DO:
                case DONT:      // IAC WILL/WONT/DO/DONT x = negotiating option x
                    will=*bp;
                    bp++;
                    state=2;
                    break;
                case SB:        // IAC SB x = subnegotiations of option x
                    bp++;
                    state=3;
                    break;
                default:
                    bp++;
                    state=0;
                }
                break;
            case 2: // IAC WILL/WONT/DO/DONT
                neg[2]=*bp;
                switch (*bp)
                {
                case ECHO:
                    switch (will)
                    {
                        case WILL:  neg[1]=DO;   break; // makes servers happy
                        case DO:    neg[1]=WONT; break;
                        case WONT:  neg[1]=DONT; break;
                        case DONT:  neg[1]=WONT; break;
                    }
                    break;
                case SUPPRESS_GO_AHEAD:
                    switch (will)
                    {
                        case WILL:  neg[1]=DO;   break;
                        case DO:    neg[1]=WILL; break;
                        case WONT:  neg[1]=DONT; break;
                        case DONT:  neg[1]=WONT; break;
                    }
                    break;
                default:
                    switch (will)
                    {
                        case WILL:  neg[1]=DONT; break;
                        case DO:    neg[1]=WONT; break;
                        case WONT:  neg[1]=DONT; break;
                        case DONT:  neg[1]=WONT; break;
                    }
                }
                send(sock, (char*)neg, 3, 0);
                bp++;
                state=0;
                break;
            case 3: // IAC SB
                bp++;
                state=4;
                sublen=0;
                break;
            case 4: // IAC SB x ...
                if (*bp++==IAC)
                    state=5;
                else if (sublen++>=64)  // probably an unterminated sequence
                    state=0;
                else
                    state=4;
                break;
            case 5: // IAC SB x ... IAC
                if (*bp++==SE)
                    state=0;
                else
                    state=4;
            }
        }
        if (write(fd, (char*)out, op-out)!=op-out)
            break;
    }

    close(sock);
    close(fd);
}


int open_telnet(char* url, int mode, char **error)
{
    int fd;
    char *rest;

    if (mode&SM_WRITE)
    {
        *error="Writing to telnet streams is not supported (yet?)";
        return -1;
    }

    fd=connect_tcp(url, 23, &rest, error);
    if (fd==-1)
        return -1;

    // we may write the rest of the URL to the socket here ...

    return filter(telnet, fd, !!(mode&SM_WRITE), 0, error);
}
