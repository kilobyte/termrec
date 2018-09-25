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
            switch (rnd(8))
            {
            case 0:
                printf("\e%c", rndchar("78DEM"));
                break;
            default:
                printf("\e[%d;%d%c", rnd(5), rnd(5), rndchar("mDCFAEBrJKLM@PXfGdc"));
            }
        }
    }
}
