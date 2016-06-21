#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "export.h"
#include "stream.h"
#include "gettext.h"


#define EAT_COLOUR \
    if (*rp=='\e')                                      \
    {                                                   \
        rp++;                                           \
        if (*rp++!='[')                                 \
            return 0;                                   \
        while ((*rp>='0' && *rp<='9') || *rp==';')      \
            rp++;                                       \
        if (*rp++!='m')                                 \
            return 0;                                   \
    }

static int match(char *rp, char *rest)
{       // pattern is /\r\n\e\[\d+d ([a-z])\) $rest (\e\[[0-9;]*m)?\(/
    char res;

    if (*rp++!='\r')
        return 0;
    if (*rp++!='\n')
        return 0;
    if (*rp++!='\e')
        return 0;
    if (*rp++!='[')
        return 0;
    while (*rp>='0' && *rp<='9')
        rp++;
    if (*rp++!='d')
        return 0;
    if (*rp==' ')
        rp++;
    res=*rp++;
    if (*rp++!=')')
        return 0;
    if (*rp++!=' ')
        return 0;
    EAT_COLOUR;
    while (*rest)
        if (*rp++!=*rest++)
            return 0;
    EAT_COLOUR;
    if (*rp++!=' ')
        return 0;
    EAT_COLOUR;
    if (*rp++!='(')
        return 0;
    return res;
}


struct filterarg
{
    int sock;
    char *rest;
};


static void termcast(int in, int out, char *arg)
{
    char buf[BUFSIZ+64], *cp, ses;
    char *rp;   // potential \r
    int len, inbuf;
    int sock=((struct filterarg *)arg)->sock;
    char *rest=((struct filterarg *)arg)->rest;
    free(arg);

    inbuf=0;
    rp=buf;
    while (1)
    {
        if (rp!=buf)
        {
            memmove(buf, rp, buf-rp+inbuf);
            inbuf-=rp-buf;
            rp=buf;
        }
        else if (inbuf>BUFSIZ/2)
        {
            memcpy(buf, buf+(inbuf+1)/2, inbuf/2);
            inbuf/=2;
        }
        len=read(in, buf+inbuf, BUFSIZ-inbuf);
        if (len<=0)
        {
            if (len<0 && errno)
                len=snprintf(buf, BUFSIZ, "\e[0m%s\n", strerror(errno));
            else
                len=snprintf(buf, BUFSIZ, "\e[0m%s\n", _("Input terminated."));
            if (write(out, buf, len))
                {}; // ignore the result, we're failing already
            free(rest);
            goto end;
        }
        if (write(out, buf+inbuf, len) != len)
        {
            free(rest);
            goto end;
        }
        inbuf+=len;
        memset(buf+inbuf, 0, 64);

        // try screen-scraping
        cp=rp;
        do if ((ses=match(cp, rest)))
            goto found;
          while ((cp=strchr((rp=cp)+1, '\r')));

        // TODO: press space every some time
    }
found:
    free(rest);
    if (write(sock, &ses, 1) != 1)
        return;
    while ((len=read(in, buf, BUFSIZ))>0)
        if (write(out, buf, len)!=len)
            break;;

    // Alas, there's no real way to detect when the session ends, other
    // than a server disconnect.
end:
    close(sock);
    close(out);
}


int open_termcast(char* url, int mode, char **error)
{
    int fd, fdmid;
    char *rest;

    if (mode&SM_WRITE)
    {
        *error="Writing to termcast streams is not supported (yet?)";
        return -1;
    }

    if ((fd=connect_tcp(url, 23, &rest, error))==-1)
        return -1;
    if (!rest || *rest++!='/' || !*rest)
    {
        close(fd);
        *error=_("What termcast session to look for?");
        return -1;
    }
    if ((fdmid=filter(telnet, fd, !!(mode&SM_WRITE), 0, error))==-1)
        return -1;
    struct filterarg *fa = malloc(sizeof(struct filterarg));
    fa->sock=fd;
    fa->rest=strdup(rest);
    return filter(termcast, fdmid, !!(mode&SM_WRITE), (char*)fa, error);
}
