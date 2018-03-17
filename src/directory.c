// Low level directory interface

#include "directory.h"

// Build directory stream
struct directory* directory_create(const char* path) {
  struct directory* base = malloc(sizeof(struct directory));
#ifdef _WINDOWS
  char* path_file = combine_string(path, "/*.*");
  wchar_t* os = string_system(path_file);
  base->dir = FindFirstFileW(os, &base->entry);
  free(os);
  free(path_file);
  base->next = 0;
  base->last_name = NULL;
#else
  base->dir = opendir(path);
#endif
  return base;
}

// Get name of next directory entry
const char* directory_next(struct directory* base) {
#ifdef _WINDOWS
  if (base->dir==INVALID_HANDLE_VALUE) {
    return NULL;
  }

  if (base->next && FindNextFileW(base->dir, &base->entry)==0) {
    return NULL;
  }

  base->next = 1;
  free(base->last_name);
  base->last_name = string_internal(&base->entry.cFileName[0]);
  return base->last_name;
#else
  if (!base->dir) {
    return NULL;
  }

  base->entry = readdir(base->dir);
  if (!base->entry) {
    return NULL;
  }

  return &base->entry->d_name[0];
#endif
}

// Close directory stream
void directory_destroy(struct directory* base) {
#ifdef _WINDOWS
  free(base->last_name);
  if (base->dir!=INVALID_HANDLE_VALUE) {
    FindClose(base->dir);
  }
#else
  if (base->dir) {
    closedir(base->dir);
  }
#endif

  free(base);
}
