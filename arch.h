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

/*In Win32 we can't simply cancel a thread, so we have to use
  a mutex.
*/
extern HANDLE changes;
extern int cancel;
extern HANDLE pth;

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

void synch_init_wait();
void synch_wait(struct timeval *tv);
void synch_speed(int sp);
void synch_start(size_t where, int arg);
