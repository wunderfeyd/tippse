// Tippse - threading - create/destroy threads

#include "thread.h"

void thread_create_inplace(struct thread* thread, thread_callback callback, void* data) {
  thread->callback = callback;
  thread->data = data;
  thread->shutdown = 0;
  thread->destroyed = 0;
#ifdef _WINDOWS
  DWORD tid;
  thread->handle = CreateThread(NULL, 0, thread_entry, (void*)thread, 0, &tid);
#else
  pthread_create(&thread->handle, NULL, thread_entry, (void*)thread);
#endif
}

void thread_destroy_inplace(struct thread* thread) {
#ifdef _WINDOWS
  WaitForSingleObject(thread->handle, INFINITE);
  CloseHandle(thread->handle);
#else
 	pthread_join(thread->handle, NULL);
  pthread_detach(thread->handle);
#endif
}

void thread_shutdown(struct thread* thread) {
  thread->shutdown = 1;
}

#ifdef _WINDOWS
DWORD WINAPI thread_entry(void* data) {
#else
void* thread_entry(void* data) {
#endif
  struct thread* thread = (struct thread*)data;
  thread->callback(thread);
  thread->destroyed = 1;
#ifdef _WINDOWS
  return 0;
#else
  return NULL;
#endif
}
