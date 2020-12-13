// Tippse - mutex - mutex handling

#include "mutex.h"

void mutex_create_inplace(struct mutex* mutex) {
#ifdef _WINDOWS
  InitializeCriticalSection(&mutex->handle);
#else
  pthread_mutexattr_init(&mutex->attribute);
#ifdef __APPLE__
  pthread_mutexattr_settype(&mutex->attribute, PTHREAD_MUTEX_ADAPTIVE_NP);
#else
  pthread_mutexattr_settype(&mutex->attribute, PTHREAD_MUTEX_NORMAL);
#endif
  pthread_mutex_init(&mutex->handle, &mutex->attribute);
#endif
}

void mutex_destroy_inplace(struct mutex* mutex) {
#ifdef _WINDOWS
  DeleteCriticalSection(&mutex->handle);
#else
  pthread_mutex_destroy(&mutex->handle);
  pthread_mutexattr_destroy(&mutex->attribute);
#endif
}

void mutex_lock(struct mutex* mutex) {
#ifdef _WINDOWS
  EnterCriticalSection(&mutex->handle);
#else
  pthread_mutex_lock(&mutex->handle);
#endif
}

void mutex_unlock(struct mutex* mutex) {
#ifdef _WINDOWS
  LeaveCriticalSection(&mutex->handle);
#else
  pthread_mutex_unlock(&mutex->handle);
#endif
}

int mutex_trylock(struct mutex* mutex) {
#ifdef _WINDOWS
  return TryEnterCriticalSection(&mutex->handle)?1:0;
#else
  return pthread_mutex_trylock(&mutex->handle)==0?1:0;
#endif
}
