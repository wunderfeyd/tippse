// Low level file interface

#include "file.h"

#include "misc.h"

// Open a file and create base structure if opening succeded
struct file* file_create(const char* path, int flags) {
#ifdef _WINDOWS
  DWORD creation = 0;
  if (flags&TIPPSE_FILE_CREATE) {
    if (flags&TIPPSE_FILE_TRUNCATE) {
      creation = CREATE_ALWAYS;
    } else {
      creation = OPEN_ALWAYS;
    }
  } else {
    if (flags&TIPPSE_FILE_TRUNCATE) {
      creation = TRUNCATE_EXISTING;
    } else {
      creation = OPEN_EXISTING;
    }
  }

  DWORD access = 0;
  DWORD share = 0;
  if (flags&TIPPSE_FILE_WRITE) {
    access |= GENERIC_WRITE;
    share |= FILE_SHARE_WRITE;
  }
  if (flags&TIPPSE_FILE_READ) {
    access |= GENERIC_READ;
    share |= FILE_SHARE_READ;
  }

  wchar_t* os = string_system(path);
  HANDLE fd = CreateFileW(os, access, share, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
  free(os);
  if (fd==INVALID_HANDLE_VALUE) {
    return NULL;
  }
#else
  int conv = 0;
  if (flags&TIPPSE_FILE_TRUNCATE) {
    conv |= O_TRUNC;
  }
  if (flags&TIPPSE_FILE_CREATE) {
    conv |= O_CREAT;
  }
  if ((flags&(TIPPSE_FILE_READ|TIPPSE_FILE_WRITE))==(TIPPSE_FILE_READ|TIPPSE_FILE_WRITE)) {
    conv |= O_RDWR;
  } else if (flags&TIPPSE_FILE_WRITE) {
    conv |= O_WRONLY;
  } else if (flags&TIPPSE_FILE_READ) {
    conv |= O_RDONLY;
  }

  int fd = open(path, conv, 0644);
  if (fd==-1) {
    return NULL;
  }
#endif
  struct file* base = malloc(sizeof(struct file));
  base->fd = fd;
  return base;
}

// Close file structure
void file_destroy(struct file* base) {
#ifdef _WINDOWS
  if (base->fd!=INVALID_HANDLE_VALUE) {
    CloseHandle(base->fd);
  }
#else
  if (base->fd!=-1) {
    close(base->fd);
  }
#endif
  free(base);
}

// Read from file
size_t file_read(struct file* base, void* buffer, size_t length) {
#ifdef _WINDOWS
  DWORD read;
  if (!ReadFile(base->fd, buffer, (DWORD)length, &read, NULL)) {
    return 0;
  }
  return (size_t)read;
#else
  ssize_t ret = read(base->fd, buffer, length);
  if (errno==EPERM) {
    ret = (ssize_t)length;
  }
  return (ret>=0)?(size_t)ret:0;
#endif
}

// Write to file
size_t file_write(struct file* base, void* buffer, size_t length) {
#ifdef _WINDOWS
  DWORD written;
  if (!WriteFile(base->fd, buffer, (DWORD)length, &written, NULL)) {
    return 0;
  }
  return (size_t)written;
#else
  ssize_t ret = write(base->fd, buffer, length);
  return (ret>=0)?(size_t)ret:0;
#endif
}

file_offset_t file_seek(struct file* base, file_offset_t offset, int relative) {
#ifdef _WINDOWS
  LONG low = (LONG)offset;
  LONG high = (LONG)(offset>>32);
  low = (LONG)SetFilePointer(base->fd, low, &high, (DWORD)relative);
  return (((file_offset_t)high)<<32)|((file_offset_t)low);
#else
  return (file_offset_t)lseek(base->fd, (off_t)offset, relative);
#endif
}
