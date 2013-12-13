#include "config.h"
# include <netdb.h>
# include <pthread.h>
# include <unistd.h>
# include <stdlib.h>


#define mutex_t	pthread_mutex_t
#define mutex_lock(x) pthread_mutex_lock(&x)
#define mutex_unlock(x) pthread_mutex_unlock(&x)
#define mutex_init(x) pthread_mutex_init(&x, 0);
#define mutex_destroy(x) pthread_mutex_destroy(&x);

#define thread_t pthread_t
#define thread_create_joinable(th,start,arg)	\
    (unix_pthread_create(th, PTHREAD_CREATE_JOINABLE, (start), (void*)(arg)))
#define thread_join(th) pthread_join(th, 0)
#define thread_create_detached(th,start,arg)	\
    (unix_pthread_create(th, PTHREAD_CREATE_DETACHED, (start), (void*)(arg)))

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

#define cond_t pthread_cond_t
#define cond_init(x) pthread_cond_init(&x, 0)
#define cond_destroy(x) pthread_cond_destroy(&x)
#define cond_wait(x,m) pthread_cond_wait(&x, &m)
#define cond_wake(x) pthread_cond_signal(&x)
