#ifndef TIPPSE_THREAD_H
#define TIPPSE_THREAD_H

#ifdef _WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#endif
#include "types.h"

struct thread;
typedef void (*thread_callback)(struct thread* thread);

struct thread {
#ifdef _WINDOWS
  HANDLE handle;
#else
  pthread_t handle;
#endif
  thread_callback callback;
  void* data;
  int shutdown;
  int destroyed;
};

void thread_create_inplace(struct thread* thread, thread_callback callback, void* data);
void thread_destroy_inplace(struct thread* thread);
void thread_shutdown(struct thread* thread);
#ifdef _WINDOWS
DWORD WINAPI thread_entry(void* data);
#else
void* thread_entry(void* data);
#endif

#endif /* #ifndef TIPPSE_THREAD_H */
