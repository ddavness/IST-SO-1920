/*

    File: locks.h
    Description: Describes functions that handle lock switches, while
    also handling errors.

*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "color.h"
#include "locks.h"

void errWrap(int errCondition, char* errMsg) {
    if (errCondition) {
        fprintf(stderr, red_bold("%s"), errMsg);
        perror("\nError");
        exit(EXIT_FAILURE);
    }
}

void mutex_init(pthread_mutex_t* mutex) {
    errWrap(pthread_mutex_init(mutex, NULL), "Could not initialize the mutex!");
}
void mutex_lock(pthread_mutex_t* mutex) {
    errWrap(pthread_mutex_lock(mutex), "Failed to lock the mutex!");
}
void mutex_unlock(pthread_mutex_t* mutex) {
    errWrap(pthread_mutex_unlock(mutex), "Failed to unlock the mutex!");
}
void mutex_destroy(pthread_mutex_t* mutex) {
    errWrap(pthread_mutex_destroy(mutex), "Failed to destroy the mutex!");
}

void rwlock_init(pthread_rwlock_t* lock) {
    errWrap(pthread_rwlock_init(lock, NULL), "Could not initialize the R/W Lock!");
}
void rwlock_rdlock(pthread_rwlock_t* lock) {
    errWrap(pthread_rwlock_rdlock(lock), "Failed to lock the R/W Lock!");
}
void rwlock_wrlock(pthread_rwlock_t* lock) {
    errWrap(pthread_rwlock_wrlock(lock), "Failed to lock the R/W Lock!");
}
void rwlock_unlock(pthread_rwlock_t* lock) {
    errWrap(pthread_rwlock_unlock(lock), "Failed to unlock the R/W Lock!");
}
void rwlock_destroy(pthread_rwlock_t* lock) {
    errWrap(pthread_rwlock_destroy(lock), "Could not destroy the R/W Lock!");
}