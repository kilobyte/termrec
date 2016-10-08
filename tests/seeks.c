#include <stdio.h>
#include <string.h>
#include <ttyrec.h>
#include "sys/error.h"

ttyrec tr;

#define BUFFER_SIZE 64

static void sub(int t1, int t2, const char *exp)
{
    char buf[BUFFER_SIZE], *bptr;
    ttyrec_frame fr;
    struct timeval tv;

    printf("Exp: [%s]\n", exp);

    tv.tv_sec=t1;
    tv.tv_usec=0;
    fr=ttyrec_seek(tr, (t1!=-1)?&tv:0, 0);
    bptr=buf;
    while (fr && (t2==-1 || fr->t.tv_sec<=t2))
    {
        if (bptr+10>buf+BUFFER_SIZE)
            die("Buffer overflow!\n");
        if (bptr!=buf)
            *bptr++=' ';
        bptr+=snprintf(bptr, 8, "%.*s", fr->len, fr->data);
        fr=ttyrec_next_frame(tr, fr);
    }
    printf("Got: [%s]\n", buf);
    if (strcmp(buf, exp))
        die(" `-- mismatch!\n");
}

int main()
{
    struct timeval tv;
    int i;
    char word[2]="x";

    tr=ttyrec_init(0);
    if (!tr)
        die("ttyrec_init() failed");
    tv.tv_sec=1;
    tv.tv_usec=0;

    for (i=1;i<=10;i++)
    {
        word[0]=i+'A'-1;
        ttyrec_add_frame(tr, &tv, word, 1);
    }

    sub( 1,10, "A B C D E F G H I J");
    sub(-1,10, "A B C D E F G H I J");
    sub( 1,-1, "A B C D E F G H I J");
    sub(-1,-1, "A B C D E F G H I J");
    sub( 2, 3, "B C");
    sub( 9,-1, "I J");
    sub( 0, 1, "A");

    ttyrec_free(tr);
    return 0;
}
