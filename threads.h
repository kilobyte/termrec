#include "config.h"
#ifdef IS_WIN32
# include <winsock2.h>
# include <windows.h>
#else
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <pthread.h>
# include <unistd.h>
# include <stdlib.h>
#endif

#ifdef IS_WIN32

#define mutex_t	CRITICAL_SECTION
#define mutex_lock(x) EnterCriticalSection(&x)
#define mutex_unlock(x) LeaveCriticalSection(&x)
#define mutex_init(x) InitializeCriticalSection(&x);
#define mutex_destroy(x) DeleteCriticalSection(&x);

#define thread_t HANDLE
#define thread_create_joinable(th,start,arg)	\
    (!(th=CreateThread(0, 4096, (LPTHREAD_START_ROUTINE)(start), (void*)(arg), 0, (LPDWORD)&th)))
#define thread_join(th)				\
    {						\
        WaitForSingleObject(th, INFINITE);	\
        CloseHandle(th);			\
    }
#define thread_create_detached(th,start,arg)	\
    (win32_thread_create_detached(&(th), (LPTHREAD_START_ROUTINE)(start), (void*)(arg)))
    
static inline int win32_thread_create_detached(thread_t *th, LPTHREAD_START_ROUTINE start, void *arg)
{
    DWORD dummy;
    
    if (!(*th=CreateThread(0, 0/*4096*/, (LPTHREAD_START_ROUTINE)start, arg, 0, &dummy)))
            error("CreateThread() failed.\n");
    CloseHandle(*th);
    return !*th;
}

static inline void sockets_init()
{
    WSADATA wsaData;

    WSAStartup(MAKEWORD(2,2), &wsaData);
}

#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND

/* Win32 */
#else
/* Unix */

#define mutex_t	pthread_mutex_t
#define mutex_lock(x) pthread_mutex_lock(&x)
#define mutex_unlock(x) pthread_mutex_unlock(&x)
#define mutex_init(x) pthread_mutex_init(&x, 0);
#define mutex_destroy(x) pthread_mutex_destroy(&x);

#define thread_t pthread_t
#define thread_create_joinable(th,start,arg)	\
    (unix_pthread_create(&(th), PTHREAD_CREATE_JOINABLE, (start), (void*)(arg)))
#define thread_join(th) pthread_join(th, 0)
#define thread_create_detached(th,start,arg)	\
    (unix_pthread_create(&(th), PTHREAD_CREATE_DETACHED, (start), (void*)(arg)))

#define sockets_init()
#define closesocket(x) close(x)

static inline int unix_pthread_create(pthread_t *th, int det, void *start, void *arg)
{
    pthread_attr_t attr;
    int ret;
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, det);
    ret=pthread_create(th, &attr, start, arg);
    pthread_attr_destroy(&attr);
    
    return ret;
}

/* Unix */
#endif
