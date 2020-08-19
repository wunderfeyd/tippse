#ifndef TIPPSE_FILE_H
#define TIPPSE_FILE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "types.h"

#define TIPPSE_FILE_CREATE 1
#define TIPPSE_FILE_TRUNCATE 2
#define TIPPSE_FILE_READ 4
#define TIPPSE_FILE_WRITE 8

#ifdef _WINDOWS
#define TIPPSE_SEEK_CURRENT FILE_CURRENT
#define TIPPSE_SEEK_START FILE_BEGIN
#define TIPPSE_SEEK_END FILE_END
#else
#define TIPPSE_SEEK_CURRENT SEEK_CUR
#define TIPPSE_SEEK_START SEEK_SET
#define TIPPSE_SEEK_END SEEK_END
#endif

struct file {
#ifdef _WINDOWS
  HANDLE fd;           // file stream handle
#else
  int fd;              // file stream
#endif
};

struct file* file_create(const char* path, int flags);
size_t file_read(struct file* base, void* buffer, size_t length);
size_t file_write(struct file* base, void* buffer, size_t length);
file_offset_t file_seek(struct file* base, file_offset_t offset, int relative);
void file_destroy(struct file* base);

#endif // #ifndef TIPPSE_FILE_H
