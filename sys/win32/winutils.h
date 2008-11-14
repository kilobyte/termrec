#include <windows.h>

int message_loop(HANDLE* lphObjects, int cObjects);
#undef stderr
#define stderr stdout
