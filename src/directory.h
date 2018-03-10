#ifndef TIPPSE_DIRECTORY_H
#define TIPPSE_DIRECTORY_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#endif

struct directory {
#ifdef _WINDOWS
  HANDLE dir;           // directory stream handle
  WIN32_FIND_DATA entry; // last read directory entry
  int next;             // fetch next entry
#else
  DIR* dir;             // directory stream
  struct dirent* entry; // last read directory entry
#endif
};

#endif // #ifndef TIPPSE_DIRECTORY_H
