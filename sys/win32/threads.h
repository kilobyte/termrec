#include <windows.h>

#define mutex_t	CRITICAL_SECTION
#define mutex_lock(x) EnterCriticalSection(&x)
#define mutex_unlock(x) LeaveCriticalSection(&x)
#define mutex_init(x) InitializeCriticalSection(&x);
#define mutex_destroy(x) DeleteCriticalSection(&x);

#define thread_t HANDLE
#define thread_create_joinable(th,start,arg)	\
    (!(*th=CreateThread(0, 4096, (LPTHREAD_START_ROUTINE)(start), (void*)(arg), 0, (LPDWORD)th)))
#define thread_join(th)				\
    {						\
        WaitForSingleObject(th, INFINITE);	\
        CloseHandle(th);			\
    }
#define thread_create_detached(th,start,arg)	\
    (win32_thread_create_detached(th, (LPTHREAD_START_ROUTINE)(start), (void*)(arg)))
    
static inline int win32_thread_create_detached(thread_t *th, LPTHREAD_START_ROUTINE start, void *arg)
{
    DWORD dummy;
    
    if (!(*th=CreateThread(0, 0/*4096*/, (LPTHREAD_START_ROUTINE)start, arg, 0, &dummy)))
            error("CreateThread() failed.\n");
    CloseHandle(*th);
    return !*th;
}

#define cond_t HANDLE
#define cond_init(x) {(x)=CreateEvent(0, 0, 0, 0);}
#define cond_destroy(x) CloseHandle(x);
#define cond_wait(x,m) {mutex_unlock(m);WaitForSingleObject(x, 500);mutex_lock(m);}
#define cond_wake(x) PulseEvent(x)
