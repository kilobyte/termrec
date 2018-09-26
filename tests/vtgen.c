#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int rnd(int x)
{
    if (x<=1)
        return 0;
    return rand()%x;
}

static char rndchar(const char *s)
{
    return s[rnd(strlen(s))];
}

#define ARRAYSZ(x) (sizeof(x)/sizeof(x[0]))

static const char *misc[]=
{
    "\n", "\r", "\b", "\x0c", "\t",
    "\e[?7h", "\e[?7l",
    "\e(0", "\e)0", "\e(B", "\e)B", "\x0e", "\x0f",
};

int main()
{
    while (1)
    {
        switch (rnd(4))
        {
        case 0:
            printf("a");
            break;
        case 1:
            printf("＠");
            break;
        case 2:
            printf("\xcc\x80");
            break;
        case 3:
            switch (rnd(12))
            {
            case 0:
                printf("\e%c", rndchar("78DEM"));
                break;
            case 1: case 2: case 3:
                puts(misc[rnd(ARRAYSZ(misc))]);
                break;
            default:
                printf("\e[%d;%d%c", rnd(5), rnd(5), rndchar("mDCFAEBrJKLM@PXfGdc"));
            }
        }
    }
}
