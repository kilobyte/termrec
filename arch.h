#ifdef WIN32

# include <windows.h>

#define uint32 DWORD

struct timeval {
        int      tv_sec;        /* seconds */
        int      tv_usec;  /* microseconds */
};

#define VT100_LOCK	EnterCriticalSection(&vt_mutex)
#define VT100_UNLOCK	LeaveCriticalSection(&vt_mutex)
extern CRITICAL_SECTION vt_mutex;

#else
/* Unix */

# include <sys/time.h>
# include <time.h>
#endif


/* ttyrec checks the byte sex on runtime, during _every_ swap.  Uh oh. */
#ifdef WORDS_BIGENDIAN
# define to_little_endian(x) (uint32 ( \
    ((uint32 (x) & uint32 0x000000ffU) << 24) | \
    ((uint32 (x) & uint32 0x0000ff00U) <<  8) | \
    ((uint32 (x) & uint32 0x00ff0000U) >>  8) | \
    ((uint32 (x) & uint32 0xff000000U) >> 24)))
#else
# define to_little_endian(x) (x)
#endif

void init_wait();
void timed_wait(struct timeval *tv);
