#ifndef TIPPSE_ATOMIC_H
#define TIPPSE_ATOMIC_H

#ifdef _WINDOWS
#include <windows.h>
#endif
#include "types.h"

TIPPSE_INLINE file_offset_t atomic_increment_fileoffset_t(file_offset_t* ptr) {
#ifdef _WINDOWS
  return (file_offset_t)InterlockedIncrement64((int64_t*)ptr);
#else
  return __atomic_add_fetch(ptr, 1, __ATOMIC_RELAXED);
#endif
}

TIPPSE_INLINE file_offset_t atomic_decrement_fileoffset_t(file_offset_t* ptr) {
#ifdef _WINDOWS
  return (file_offset_t)InterlockedDecrement64((int64_t*)ptr);
#else
  return __atomic_sub_fetch(ptr, 1, __ATOMIC_RELAXED);
#endif
}

#endif /* #ifndef TIPPSE_ATOMIC_H */
