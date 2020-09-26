#ifndef TIPPSE_MUTEX_H
#define TIPPSE_MUTEX_H

#ifdef _WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#endif
#include "types.h"

struct mutex {
#ifdef _WINDOWS
  CRITICAL_SECTION handle;
#else
  pthread_mutexattr_t	attribute;
  pthread_mutex_t handle;
#endif
};

void mutex_create_inplace(struct mutex* mutex);
void mutex_destroy_inplace(struct mutex* mutex);
void mutex_lock(struct mutex* mutex);
void mutex_unlock(struct mutex* mutex);
int mutex_trylock(struct mutex* mutex);

#endif /* #ifndef TIPPSE_THREADING_H */
