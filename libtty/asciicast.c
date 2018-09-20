#include "config.h"
#include "_stdint.h"
#include <stdbool.h>
#include <string.h>
#include "gettext.h"
#include "formats.h"
#include "export.h"


// Blasting many pages in a single frame is borderline legitimate, but let's
// have a sanity limit -- data that was actually recorded into separate
// frames with tiny but non-zero delays.
// Incidentally, this also allows avoiding managing a dynamic buffer.
// 1M is way too high but userspace stack is aplenty.
#define BUFFER_SIZE 1048576

#define EAT(x) do c=getc(f); while (c==' ' || c=='\t' || c=='\r' || c=='\n' x)

static bool eat_colon(FILE *f)
{
    int c;
    EAT();
    if (c != ':')
        return false;
    EAT();
    ungetc(c, f);
    return true;
}

static int64_t eat_int(FILE *f)
{
    int64_t x=0;
    while (1)
    {
        int c=getc(f);
        if (c>='0' && c<='9')
            x=x*10+c-'0';
        else
        {
            ungetc(c, f);
            return x;
        }
    }
}

// *1000000
static int64_t eat_float(FILE *f)
{
    int c;
    int64_t x=0;
    c=getc(f);
    while (c>='0' && c<='9')
        x=x*10+c-'0', c=getc(f);
    x*=1000000;

    if (c=='.')
    {
        int y=1000000;
        c=getc(f);
        while (c>='0' && c<='9')
            x+= (c-'0')*(y/=10), c=getc(f);
    }

    if (c=='e' || c=='E')
    {
        bool minus=false;
        c=getc(f);
        if (c=='+')
            c=getc(f);
        else if (c=='-')
            c=getc(f), minus=true;
        int e=0;
        while (c>='0' && c<='9')
            e=e*10+c-'0', c=getc(f);
        if (minus)
            while (e-->0)
                x/=10;
        else
            while (e-->0)
                x*=10;
    }

    ungetc(c, f);
    return x;
}

#define OUT(x) do if (--spc) *buf++=(x); else goto end; while (0)
static char* eat_string(FILE *f, char *buf)
{
    int spc=BUFFER_SIZE;
    uint16_t surrogate=0;

    while (1)
    {
        int c=getc(f);
        if (c==EOF || c=='"')
            break;
        if (c!='\\')
            OUT(c);
        else switch (c=getc(f))
        {
        case 'b':
            OUT('\b'); break;
        case 'f':
            OUT('\f'); break;
        case 'n':
            OUT('\n'); break;
        case 'r':
            OUT('\r'); break;
        case 't':
            OUT('\t'); break;
        case 'u':
            if (fscanf(f, "%4x", &c) == 1)
            {
                if (c < 0x80)
                    OUT(c);
                else if (c < 0x800)
                {
                    OUT(0xc0|c>>6);
                    OUT(0x80|c&0x3f);
                }
                else if (c < 0xD800 || c > 0xDFFF)
                {
                    OUT(0xe0|c>>12);
                    OUT(0x80|c>>6&0x3f);
                    OUT(0x80|c&0x3f);
                }
                // Note: we allow erroneous surrogate pairs separated by
                // something, silently ignore lone lead or trailing ones.
                else if (c < 0xDC00)
                    surrogate = c;
                else if (surrogate)
                {
                    c=((uint32_t)surrogate)<<10&0xffc00|c&0x3ff;
                    c+=0x10000;
                    OUT(0xf0|c>>18);
                    OUT(0x80|c>>12&0x3f);
                    OUT(0x80|c>>6&0x3f);
                    OUT(0x80|c&0x3f);
                    surrogate=0;
                }
                break;
            }
        case '"': case '\\': case '/':
        default:
            OUT(c);    break;
        }
    }
end:
    *buf=0;
    return buf;
}

#define FAIL(x) do {const char* t=(x);return synch_print(t, strlen(t), arg);} while (0)
void play_asciicast(FILE *f,
    void (*synch_init_wait)(const struct timeval *ts, void *arg),
    void (*synch_wait)(const struct timeval *tv, void *arg),
    void (*synch_print)(const char *buf, int len, void *arg),
    void *arg, const struct timeval *cont)
{
    char buf[BUFFER_SIZE];
    int bracket_level = 0;
    int version = -1;
    int c;
    int sx=80, sy=25;
    struct timeval tv;
    tv.tv_sec = tv.tv_usec = 0;

    EAT();
    if (c != '{')
        FAIL("Not an asciicast: doesn't start with a JSON object.\n");

    // Read the header.
    while (1)
    {
expect_field:
        switch (c = getc(f))
        {
        case EOF:
            FAIL("Not an asciicast: end of file within header.\n");
        case ' ': case '\t': case '\r': case '\n':
            continue;
        case '"':
            eat_string(f, buf);
            if (!eat_colon(f))
                FAIL("Not an asciicast: bad header.\n");
            if (bracket_level)
            {
skip_field:
                EAT();
                if (c==EOF)
                    FAIL("Not an asciicast: end of file within header.\n");
                else if (c=='"')
                    eat_string(f, buf);
                else if (c>='0' && c<='9')
                    ungetc(c, f), eat_float(f);
                else if (c=='{')
                {
                    bracket_level++;
                    goto expect_field;
                }
                else
                    FAIL("Not an asciicast: junk within header.\n");
            }
            else if (!strcmp(buf, "version"))
            {
                int64_t v = eat_int(f);
                if (v == 1 || v == 2)
                    version = v;
                else
                    FAIL("Unsupported asciicast version.\n");
            }
            else if (!strcmp(buf, "width"))
                sx = eat_int(f);
            else if (!strcmp(buf, "height"))
                sy = eat_int(f);
            else if (!strcmp(buf, "timestamp"))
            {
                int64_t v = eat_float(f);
                tv.tv_sec =  v/1000000;
                tv.tv_usec = v%1000000;
            }
            else if (version == 1 && !strcmp(buf, "stdout"))
            {
                if (getc(f) != '[')
                    FAIL("Not an asciicast: v1 stdout not an array.\n");
                goto body;
            }
            else
                goto skip_field;

            EAT();
            if (c == '}')
            {
                bracket_level--;
                EAT();
            }
            if (c == '}')
            {
                ungetc(c, f);
                goto expect_field;
            }
            if (c != ',')
                FAIL("Not an asciicast: junk after a JSON field.\n");
            break;
        case '}':
            if (version == 2 && !bracket_level)
                goto body;
            bracket_level--;
            break;
        default:
            FAIL("Not an asciicast: bad header.\n");
        }
    }
    /* not reached */

body:
    synch_init_wait(&tv, arg);
    synch_print(buf, sprintf(buf, "\e%%G\e[8;%d;%dt", sy, sx), arg);
    uint64_t old_delay = 0;

    while (1)
    {
        EAT(|| c==',');
        if (c == EOF || c == '}' || c == ']')
            return;
        if (c != '[')
            FAIL("Malformed asciicast: frame not an array.\n");

        EAT();
        if (c>='0' && c<='9')
            ungetc(c, f);
        else
            FAIL("Malformed asciicast: expected duration.\n");

        int64_t delay = eat_float(f);
        if (version == 2)
        {
            delay -= old_delay;
            old_delay += delay;
        }
        tv.tv_sec =  delay/1000000;
        tv.tv_usec = delay%1000000;
        synch_wait(&tv, arg);

        EAT();
        if (c != ',')
            FAIL("Malformed asciicast: no comma after duration.\n");

        if (version == 2)
        {
            EAT();
            if (c != '"')
                FAIL("Malformed asciicast: expected event type.\n");
            eat_string(f, buf);
            EAT();
            if (c != ',')
                FAIL("Malformed asciicast: no comma after event type.\n");
        }

        EAT();
        if (c !='"')
            FAIL("Malformed asciicast: expected even-data string.\n");
        synch_print(buf, eat_string(f, buf) - buf, arg);

        EAT();
        if (c !=']')
            FAIL("Malformed asciicast: event not terminated.\n");
    }
}
