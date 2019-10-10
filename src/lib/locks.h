/*

    File: locks.h
    Description: Describes functions that protect lock switches

*/

#include <pthread.h>

void mutex_init(pthread_mutex_t*);
void mutex_lock(pthread_mutex_t*);
void mutex_unlock(pthread_mutex_t*);
void mutex_destroy(pthread_mutex_t*);

void rwlock_init(pthread_rwlock_t*);
void rwlock_rdlock(pthread_rwlock_t*);
void rwlock_wrlock(pthread_rwlock_t*);
void rwlock_unlock(pthread_rwlock_t*);
void rwlock_destroy(pthread_rwlock_t*);
