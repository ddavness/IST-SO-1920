/*

    File: locks.h
    Description: Describes functions that protect lock switches

*/

#include "void.h"
#include <pthread.h>

#ifndef LOCKS
#define LOCKS

void mutex_init(pthread_mutex_t*);
void mutex_lock(pthread_mutex_t*);
void mutex_unlock(pthread_mutex_t*);
void mutex_destroy(pthread_mutex_t*);

void rwlock_init(pthread_rwlock_t*);
void rwlock_rdlock(pthread_rwlock_t*);
void rwlock_wrlock(pthread_rwlock_t*);
void rwlock_unlock(pthread_rwlock_t*);
void rwlock_destroy(pthread_rwlock_t*);

#ifdef MUTEX
    // Map macros to mutex

    typedef pthread_mutex_t lock;

    #define INIT_LOCK mutex_init
    #define LOCK_READ mutex_lock
    #define LOCK_WRITE mutex_lock
    #define LOCK_UNLOCK mutex_unlock
    #define DESTROY_LOCK mutex_destroy

    #define NOSYNC false
#elif RWLOCK
    // Map macros to RWLOCK

    typedef pthread_rwlock_t lock;
    
    #define INIT_LOCK rwlock_init
    #define LOCK_READ rwlock_rdlock
    #define LOCK_WRITE rwlock_wrlock
    #define LOCK_UNLOCK rwlock_unlock
    #define DESTROY_LOCK rwlock_destroy

    #define NOSYNC false
#else
    /*
       Nosync mode. These macros are only defined so that
       the compiler doesn't complain!
    */

    typedef void* lock;

    #define INIT_LOCK void_noarg
    #define LOCK_READ void_func
    #define LOCK_WRITE void_func
    #define LOCK_UNLOCK void_func
    #define DESTROY_LOCK void_noarg

    #define NOSYNC true
#endif

#endif